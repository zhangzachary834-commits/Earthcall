/**
 * dom_mirror.js — Earthcall Foreign Web DOM Mirror Script
 * 
 * Part of the HTML as Lexeme Formation Bridge (Zachary Zhang & GPT-5.6 Sol).
 * Maintains browser-side node identity via WeakMap, serializes bounded snapshots,
 * observes ordered mutations, and applies structured Acts with operation provenance.
 */

(function() {
    'use strict';

    if (window.__earthcall_dom_mirror_installed) {
        return;
    }
    window.__earthcall_dom_mirror_installed = true;

    const PROTOCOL_VERSION = 1;
    const BOUNDS = {
        maxNodes: 50000,
        maxTextBytesPerNode: 1000000,
        maxAttributesPerNode: 1000,
        maxMutationRecordsPerBatch: 10000
    };

    // Mint unique pageSessionId for this Document
    const sessionRandom = Math.random().toString(36).substring(2, 10);
    const pageSessionId = 'page-session.' + sessionRandom;

    // Node token mapping: WeakMap preserves identity without polluting foreign DOM with attributes
    const nodeToToken = new WeakMap();
    const tokenToNode = new Map(); // WeakMap cannot be iterated or looked up by key
    let nextTokenId = 1;
    let sequence = 0;

    function getOrCreateToken(node) {
        if (!node) return '';
        let token = nodeToToken.get(node);
        if (!token) {
            token = 'node.' + (nextTokenId++);
            nodeToToken.set(node, token);
            tokenToNode.set(token, node);
        }
        return token;
    }

    function getNodeSymbol(node) {
        if (node.nodeType === Node.ELEMENT_NODE) {
            return node.tagName.toLowerCase();
        } else if (node.nodeType === Node.TEXT_NODE) {
            return '#text';
        } else if (node.nodeType === Node.COMMENT_NODE) {
            return '#comment';
        } else if (node.nodeType === Node.DOCUMENT_NODE) {
            return '#document';
        }
        return '#unknown';
    }

    function getNodeTypeString(node) {
        switch (node.nodeType) {
            case Node.ELEMENT_NODE: return 'element';
            case Node.TEXT_NODE: return 'text';
            case Node.COMMENT_NODE: return 'comment';
            case Node.DOCUMENT_NODE: return 'document';
            default: return 'other';
        }
    }

    function postToEarthcall(type, payload) {
        const message = JSON.stringify({ type: type, payload: payload });
        if (window.webkit && window.webkit.messageHandlers && window.webkit.messageHandlers.domMirror) {
            window.webkit.messageHandlers.domMirror.postMessage(message);
        } else if (window.__earthcall_native_receiver) {
            window.__earthcall_native_receiver(message);
        } else {
            console.log('[Earthcall DomMirror]', type, payload);
        }
    }

    // Iterative snapshot serialization
    function takeSnapshot() {
        const root = document.documentElement;
        if (!root) return null;

        const rootToken = getOrCreateToken(root);
        const nodes = [];
        const queue = [{ node: root, parentToken: '', siblingIndex: 0 }];

        while (queue.length > 0 && nodes.length < BOUNDS.maxNodes) {
            const item = queue.shift();
            const curr = item.node;
            const token = getOrCreateToken(curr);

            const record = {
                nodeToken: token,
                nodeType: getNodeTypeString(curr),
                symbol: getNodeSymbol(curr),
                parentToken: item.parentToken,
                siblingIndex: item.siblingIndex,
                attributes: []
            };

            if (curr.nodeType === Node.TEXT_NODE) {
                let text = curr.textContent || '';
                if (text.length > BOUNDS.maxTextBytesPerNode) {
                    text = text.substring(0, BOUNDS.maxTextBytesPerNode);
                }
                record.textContent = text;
            } else if (curr.nodeType === Node.ELEMENT_NODE) {
                if (curr.attributes) {
                    const attrCount = Math.min(curr.attributes.length, BOUNDS.maxAttributesPerNode);
                    for (let i = 0; i < attrCount; ++i) {
                        const attr = curr.attributes[i];
                        record.attributes.push({
                            name: attr.name,
                            value: attr.value
                        });
                    }
                }

                // Enqueue children
                const childNodes = curr.childNodes;
                for (let i = 0; i < childNodes.length; ++i) {
                    const child = childNodes[i];
                    // Filter out uninteresting nodes (e.g. empty whitespace-only comments)
                    if (child.nodeType === Node.ELEMENT_NODE || child.nodeType === Node.TEXT_NODE) {
                        queue.push({
                            node: child,
                            parentToken: token,
                            siblingIndex: i
                        });
                    }
                }
            }

            nodes.push(record);
        }

        return {
            protocolVersion: PROTOCOL_VERSION,
            pageSessionId: pageSessionId,
            sequenceBase: sequence,
            url: window.location.href,
            rootNodeToken: rootToken,
            nodes: nodes
        };
    }

    // Convert MutationRecord list into DomDelta envelope
    function processMutationRecords(records, originOperationId) {
        if (!records || records.length === 0) return null;

        const deltaRecords = [];

        for (let i = 0; i < records.length && deltaRecords.length < BOUNDS.maxMutationRecordsPerBatch; ++i) {
            const rec = records[i];

            if (rec.type === 'characterData') {
                const targetToken = getOrCreateToken(rec.target);
                deltaRecords.push({
                    kind: 'text-change',
                    targetNodeToken: targetToken,
                    textContent: rec.target.textContent || ''
                });
            } else if (rec.type === 'attributes') {
                const targetToken = getOrCreateToken(rec.target);
                const attrName = rec.attributeName;
                const attrVal = rec.target.getAttribute(attrName);
                if (attrVal === null) {
                    deltaRecords.push({
                        kind: 'attribute-remove',
                        targetNodeToken: targetToken,
                        attributeName: attrName
                    });
                } else {
                    deltaRecords.push({
                        kind: 'attribute-set',
                        targetNodeToken: targetToken,
                        attributeName: attrName,
                        attributeValue: attrVal
                    });
                }
            } else if (rec.type === 'childList') {
                const parentToken = getOrCreateToken(rec.target);

                // Removed nodes
                for (let j = 0; j < rec.removedNodes.length; ++j) {
                    const remNode = rec.removedNodes[j];
                    const token = nodeToToken.get(remNode);
                    if (token) {
                        deltaRecords.push({
                            kind: 'remove',
                            targetNodeToken: token,
                            parentToken: parentToken
                        });
                        tokenToNode.delete(token);
                    }
                }

                // Added nodes
                for (let j = 0; j < rec.addedNodes.length; ++j) {
                    const addNode = rec.addedNodes[j];
                    const token = getOrCreateToken(addNode);
                    const record = {
                        nodeToken: token,
                        nodeType: getNodeTypeString(addNode),
                        symbol: getNodeSymbol(addNode),
                        parentToken: parentToken,
                        siblingIndex: Array.prototype.indexOf.call(rec.target.childNodes, addNode),
                        attributes: []
                    };
                    if (addNode.nodeType === Node.TEXT_NODE) {
                        record.textContent = addNode.textContent || '';
                    } else if (addNode.nodeType === Node.ELEMENT_NODE && addNode.attributes) {
                        for (let a = 0; a < addNode.attributes.length; ++a) {
                            record.attributes.push({
                                name: addNode.attributes[a].name,
                                value: addNode.attributes[a].value
                            });
                        }
                    }

                    deltaRecords.push({
                        kind: 'insert',
                        targetNodeToken: token,
                        parentToken: parentToken,
                        siblingIndex: record.siblingIndex,
                        node: record
                    });
                }
            }
        }

        if (deltaRecords.length === 0) return null;

        sequence += 1;
        const delta = {
            protocolVersion: PROTOCOL_VERSION,
            pageSessionId: pageSessionId,
            sequence: sequence,
            records: deltaRecords
        };
        if (originOperationId) {
            delta.originOperationId = originOperationId;
        }
        return delta;
    }

    let observer = null;

    function installObserver() {
        if (!document.documentElement) return;

        observer = new MutationObserver(function(mutations) {
            const delta = processMutationRecords(mutations, null);
            if (delta) {
                postToEarthcall('delta', delta);
            }
        });

        observer.observe(document.documentElement, {
            childList: true,
            subtree: true,
            attributes: true,
            characterData: true
        });
    }

    // Apply structured Act from Earthcall with operation provenance
    window.__earthcall_apply_act = function(actPayload) {
        let act = null;
        try {
            act = typeof actPayload === 'string' ? JSON.parse(actPayload) : actPayload;
        } catch (e) {
            return { success: false, error: 'Invalid Act JSON: ' + e.message };
        }

        if (!act || act.pageSessionId !== pageSessionId) {
            return { success: false, error: 'Session mismatch: ' + (act ? act.pageSessionId : 'null') };
        }

        const targetNode = tokenToNode.get(act.targetNodeToken);
        if (!targetNode && act.kind !== 'insertElement' && act.kind !== 'insertText') {
            return { success: false, error: 'Target node not found: ' + act.targetNodeToken };
        }

        try {
            switch (act.kind) {
                case 'setText':
                    targetNode.textContent = act.text || '';
                    break;
                case 'setAttribute':
                    targetNode.setAttribute(act.attributeName, act.attributeValue || '');
                    break;
                case 'removeAttribute':
                    targetNode.removeAttribute(act.attributeName);
                    break;
                case 'removeNode':
                    if (targetNode.parentNode) {
                        targetNode.parentNode.removeChild(targetNode);
                    }
                    break;
                case 'insertElement': {
                    const parentNode = tokenToNode.get(act.parentToken);
                    if (!parentNode) return { success: false, error: 'Parent node not found' };
                    const newEl = document.createElement(act.tagName);
                    const newToken = getOrCreateToken(newEl);
                    if (act.text) newEl.textContent = act.text;
                    if (act.attributeName && act.attributeValue) {
                        newEl.setAttribute(act.attributeName, act.attributeValue);
                    }
                    parentNode.appendChild(newEl);
                    break;
                }
                case 'insertText': {
                    const parentNode = tokenToNode.get(act.parentToken);
                    if (!parentNode) return { success: false, error: 'Parent node not found' };
                    const textNode = document.createTextNode(act.text || '');
                    getOrCreateToken(textNode);
                    parentNode.appendChild(textNode);
                    break;
                }
                case 'moveNode': {
                    const parentNode = tokenToNode.get(act.parentToken);
                    if (!parentNode) return { success: false, error: 'Parent node not found' };
                    parentNode.appendChild(targetNode);
                    break;
                }
                default:
                    return { success: false, error: 'Unknown Act kind: ' + act.kind };
            }

            // Immediately harvest synchronous mutation records for this Act
            if (observer) {
                const records = observer.takeRecords();
                const confirmationDelta = processMutationRecords(records, act.operationId);
                if (confirmationDelta) {
                    postToEarthcall('delta', confirmationDelta);
                }
            }

            return { success: true, operationId: act.operationId };
        } catch (err) {
            return { success: false, error: err.message };
        }
    };

    // Initialize snapshot on ready
    function init() {
        const snapshot = takeSnapshot();
        if (snapshot) {
            postToEarthcall('snapshot', snapshot);
            installObserver();
        }
    }

    if (document.readyState === 'complete' || document.readyState === 'interactive') {
        init();
    } else {
        window.addEventListener('DOMContentLoaded', init);
    }
})();
