"""Independent archive, CLI, preservation and concurrency witnesses.
Codex / GPT-6.1 Sol / 01a122ec-b377-7391-ad6f-86d11b501d1b / 2026-10-09.
Run: python3 -m unittest discover -s 'agent intercom' -p 'test_conversation_navigation.py'
"""
import argparse
from concurrent.futures import ThreadPoolExecutor
import json
from pathlib import Path
import subprocess
import sys
import tempfile
import unittest

import conversation_history_injection as intercom
import conversation_navigation as nav

SCRIPT = Path(intercom.__file__).resolve()


class NavigationTests(unittest.TestCase):
    def setUp(self):
        self.temp = tempfile.TemporaryDirectory()
        self.addCleanup(self.temp.cleanup)
        self.root = Path(self.temp.name).resolve()
        self.write('agent intercom/communication-threads/original.md', '# Original\n\n## Human intent\nPreserve the meaningful conversation.\n')
        self.write('docs/reflections/response.md', '# Response\n\nReplies to [the source](../../agent%20intercom/communication-threads/original.md#human-intent).\n')

    def archive(self):
        archive = nav.Archive(self.root)
        self.addCleanup(archive.close)
        return archive

    def write(self, path, text):
        p = self.root / path; p.parent.mkdir(parents=True, exist_ok=True); p.write_text(text)
        return p

    def cli(self, *args, success=True):
        result = subprocess.run([sys.executable, str(SCRIPT), 'nav', '--root', str(self.root), '--json', *args],
                                cwd=self.temp.name, text=True, capture_output=True)
        if success: self.assertEqual(result.returncode, 0, result.stderr)
        else: self.assertNotEqual(result.returncode, 0)
        return result

    def link(self, source='response.md', target='original.md', action='link'):
        return self.cli(action, source, target, '--by', 'Codex/test-session')

    def test_existing_links_reverse_and_unicode_fts(self):
        self.write('docs/Unicode.md', '# 创造\nAn unusual amber constellation.\n')
        archive = self.archive()
        self.assertEqual(archive.find('amber constellation', 2)[0]['path'], 'docs/Unicode.md')
        incoming = nav.connected(archive, archive.select('original.md'))
        self.assertEqual(incoming[0]['source'], 'docs/reflections/response.md')
        self.assertEqual(incoming[0]['target'].split('#')[1], 'human-intent')
        self.assertFalse(archive.issues)

    def test_cli_link_trace_is_bidirectional_and_preserves_sources(self):
        original = {p:p.read_bytes() for p in nav.corpus(self.root)}
        self.link(target='original.md#human-intent')
        trace = json.loads(self.cli('trace', 'original.md').stdout)
        self.assertEqual({n['path'] for n in trace['nodes']}, {str(p.relative_to(self.root)) for p in original})
        self.assertEqual(trace['edges'][0]['relation'], 'responds-to')
        show = json.loads(self.cli('show', 'original.md#human-intent', '--lines', '2').stdout)
        self.assertEqual(show['start'], 3)
        self.assertEqual(show['connections'][0]['origin'], 'journal')
        self.assertTrue(all(p.read_bytes()==data for p,data in original.items()))

    def test_duplicate_links_and_withdrawals_are_append_only(self):
        self.link(); journal = self.root/nav.JOURNAL; first=journal.read_bytes()
        self.assertFalse(json.loads(self.link().stdout)['changed'])
        self.assertEqual(first,journal.read_bytes())
        self.link(action='unlink'); self.assertTrue(journal.read_bytes().startswith(first))
        self.assertEqual(len(nav.trace(self.archive(), 'docs/reflections/response.md', 10)['nodes']),1)
        self.link(); self.assertEqual(len(nav.read_journal(journal)),3)

    def test_ambiguity_missing_anchors_and_escape_refuse(self):
        self.write('docs/other/original.md', '# Another original\n')
        self.cli('show','original.md',success=False)
        self.cli('link','response.md','original.md#absent','--by','test',success=False)
        self.cli('link','response.md','../outside.md','--by','test',success=False)
        self.assertFalse((self.root/nav.JOURNAL).exists())

    def test_invalid_journal_is_not_extended(self):
        self.link(); journal=self.root/nav.JOURNAL; journal.write_bytes(journal.read_bytes()+b'{"unfinished":')
        prior=journal.read_bytes();self.link_failure()
        self.assertEqual(journal.read_bytes(),prior)

    def link_failure(self):
        self.cli('link','response.md','original.md','--by','test',success=False)

    def damaged_journal(self):
        """A sound link, a replay of it, and a foreign record, as found 2026-11-04."""
        self.link(); journal=self.root/nav.JOURNAL; sound=journal.read_bytes()
        foreign=b'{"timestamp":"2026-11-02","phase":"P16_REVIEW","summary":"keep me"}\n'
        journal.write_bytes(sound+sound+foreign)
        return journal,sound,foreign

    def test_reads_survive_damage_loudly_and_writes_still_refuse(self):
        journal,_,_=self.damaged_journal(); prior=journal.read_bytes()
        found=self.cli('find','response')
        self.assertTrue(json.loads(found.stdout))
        self.assertIn('2 damaged line(s), first at line 2',found.stderr)
        self.assertIn('nav repair',found.stderr)
        trace=json.loads(self.cli('trace','original.md').stdout)
        self.assertEqual(len(trace['edges']),1)
        self.assertEqual(json.loads(self.cli('doctor',success=False).stdout)['journal_damage'],2)
        refused=self.cli('link','response.md','original.md','--by','test',success=False)
        self.assertIn('nav repair',refused.stderr)
        self.assertEqual(journal.read_bytes(),prior)

    def test_repair_previews_then_keeps_exact_bytes_and_quarantines_foreign_lines(self):
        journal,sound,foreign=self.damaged_journal(); prior=journal.read_bytes()
        preview=json.loads(self.cli('repair').stdout)
        self.assertEqual((preview['replayed'],preview['replayed_lines']),(1,'2'))
        self.assertEqual((preview['quarantined'],preview['applied']),(1,False))
        self.assertEqual(journal.read_bytes(),prior)
        self.cli('repair','--apply',success=False)  # attribution is required
        self.assertEqual(journal.read_bytes(),prior)
        applied=json.loads(self.cli('repair','--apply','--by','test/session').stdout)
        self.assertTrue(applied['applied'])
        self.assertEqual(journal.read_bytes(),sound)
        held=json.loads((self.root/nav.QUARANTINE).read_text())
        self.assertEqual(held['raw'].encode()+b'\n',foreign)
        self.assertEqual((held['by'],held['line']),('test/session',3))
        # Repaired means usable, and a second repair has nothing to do.
        self.write('docs/another.md','# Another\n')
        self.cli('link','another.md','original.md','--by','test')
        self.assertFalse(json.loads(self.cli('repair','--apply','--by','test').stdout)['applied'])
        self.assertEqual(len((self.root/nav.QUARANTINE).read_text().splitlines()),1)

    def test_a_replay_cannot_revive_a_withdrawn_edge(self):
        self.link(); journal=self.root/nav.JOURNAL; connect=journal.read_bytes()
        self.link(action='unlink'); withdrawn=journal.read_bytes()
        journal.write_bytes(withdrawn+connect)
        self.assertEqual(json.loads(self.cli('trace','original.md').stdout)['edges'],[])
        self.cli('repair','--apply','--by','test')
        self.assertEqual(journal.read_bytes(),withdrawn)

    def test_repair_refuses_conflicting_records(self):
        self.link(); journal=self.root/nav.JOURNAL; entry=json.loads(journal.read_text())
        entry['note']='a different story under the same id'
        journal.write_bytes(journal.read_bytes()+json.dumps(entry).encode()+b'\n'); prior=journal.read_bytes()
        self.assertEqual(json.loads(self.cli('repair').stdout)['conflicts'],'2')
        self.cli('repair','--apply','--by','test',success=False)
        self.assertEqual(journal.read_bytes(),prior)
        self.assertFalse((self.root/nav.QUARANTINE).exists())

    def test_repair_completes_a_sound_tail_and_quarantines_a_torn_one(self):
        self.link(); journal=self.root/nav.JOURNAL; sound=journal.read_bytes()
        journal.write_bytes(sound[:-1])
        self.assertTrue(json.loads(self.cli('repair').stdout)['incomplete_tail'])
        self.cli('repair','--apply','--by','test')
        self.assertEqual(journal.read_bytes(),sound)
        journal.write_bytes(sound+b'{"unfinished":')
        self.cli('repair','--apply','--by','test')
        self.assertEqual(journal.read_bytes(),sound)
        self.assertEqual(json.loads((self.root/nav.QUARANTINE).read_text())['raw'],'{"unfinished":')

    def test_this_checkouts_journal_is_sound(self):
        # The loud alarm: when a merge or a hand-written append damages the real
        # journal, CI fails here. Fix with: nav repair, then nav repair --apply --by ...
        self.assertTrue(nav.read_journal(nav.ROOT/nav.JOURNAL))

    def test_process_concurrency_deduplicates_and_retains_all_distinct_edges(self):
        for n in range(6): self.write(f'docs/response-{n}.md',f'# Response {n}\n')
        def submit(n): return self.cli('link',f'response-{n%6}.md','original.md','--by',f'test/{n}')
        with ThreadPoolExecutor(max_workers=6) as pool: list(pool.map(submit,range(12)))
        records=nav.read_journal(self.root/nav.JOURNAL)
        self.assertEqual(len(records),6)
        self.assertEqual(len(nav.active_records(records)),6)

    def test_bounded_cycle_trace(self):
        self.link();self.link('original.md','response.md')
        archive=self.archive()
        self.assertEqual(len(nav.trace(archive,archive.select('original.md'),10)['nodes']),2)
        result=nav.trace(archive,archive.select('original.md'),1)
        self.assertTrue(result['truncated']);self.assertEqual(len(result['nodes']),1)

    def test_legacy_cleanup_preserves_every_byte_outside_blocks_and_restores_jsonl(self):
        log=self.root/'agent intercom/communication-threads/log.md'
        message=intercom.append_message(log,'maker/session','*','meaning survives')
        body=log.read_bytes()
        block=b'<!-- NAV_BLOCK_START -->\r\n> **Thread Navigation: Legacy**\r\n> [View Full Thread Index](00_THREAD_INDEX.md)\r\n<!-- NAV_BLOCK_END -->'
        raw=block+b'\n\n'+body;log.write_bytes(raw)
        archive=self.archive()
        args=argparse.Namespace(apply=False,by='test/session')
        self.assertEqual(nav.tidy(archive,args)['files'],1);self.assertEqual(log.read_bytes(),raw)
        args.apply=True;result=nav.tidy(archive,args)
        self.assertEqual(log.read_bytes(),b'\n\n'+body)
        self.assertEqual((self.root/result['backup']/log.relative_to(self.root)).read_bytes(),raw)
        self.assertEqual(intercom.read_messages(log)[0]['id'],message['id'])
        self.assertEqual(nav.tidy(self.archive(),args)['files'],0)

    def test_jsonl_message_anchors_and_foreign_cwd(self):
        log=self.root/'agent intercom/communication-threads/log.txt'
        message=intercom.append_message(log,'maker/session','*','a real event')
        show=json.loads(self.cli('show','log.txt#'+message['id']).stdout)
        self.assertEqual(show['start'],1)
        self.assertEqual(json.loads(show['excerpt'][0]['text'])['id'],message['id'])

    def test_markdown_balanced_paths_reference_links_and_ignored_code(self):
        self.write('docs/with (parentheses).md','# Parenthesized\n')
        self.write('docs/linked.md','# Links\n[one](with%20(parentheses).md)\n[two][parent]\n[parent]: <with%20(parentheses).md>\n```\n[example](not-real.md)\n```\n')
        archive=self.archive()
        edges=[e for e in archive.edges if e['source']=='docs/linked.md']
        self.assertEqual(len(edges),1);self.assertFalse(archive.issues)

    def test_browser_is_self_contained_escaped_and_does_not_overwrite_sources(self):
        self.write('docs/hostile.md','# Hostile\n</script><script>alert(1)</script>\n')
        self.link(); output=json.loads(self.cli('browse','--output','scratch/map.html').stdout)
        html=Path(output['output']).read_text()
        self.assertNotIn('</script><script>alert(1)</script>',html)
        payload=json.loads(html.split('<script id="archive" type="application/json">',1)[1].split('</script>',1)[0])
        self.assertEqual(len(payload['docs']),3)
        self.assertEqual(len([e for e in payload['edges'] if e['origin']=='journal']),1)
        self.assertIn('meaningful conversation',html)

    def test_search_and_excerpt_limits(self):
        result=json.loads(self.cli('find','response','--limit','1').stdout);self.assertEqual(len(result),1)
        self.cli('show','original.md','--lines','0',success=False)
        self.cli('trace','original.md','--limit','201',success=False)
        self.write('docs/long.md','# Long\n'+'line\n'*400)
        result=json.loads(self.cli('show','long.md','--start','350','--lines','2').stdout)
        self.assertEqual([l['line'] for l in result['excerpt']],[350,351])

    def test_heading_anchors_ignore_code_and_keep_duplicate_suffixes(self):
        text = '# Real\n```sh\n# Fake\n```\n## Real\n<a id="named"></a>\n'
        self.assertEqual(nav.anchors(text), {'real':1, 'real-1':5, 'named':6})

    def test_existing_source_artifacts_and_inline_commands_are_not_missing_docs(self):
        self.write('examples/a.txt', 'A valid artifact outside the document corpus.')
        self.write('CMakeLists.txt', '# Build')
        self.write('docs/usage.md', '# Usage\n[Example](../examples/a.txt) and [build](../CMakeLists.txt).\n`nav link "response.md" "parent.md#section" --by "session"`\n')
        self.assertFalse(self.archive().issues)

    def test_thread_metadata_does_not_become_a_chat(self):
        original=intercom.THREADS_DIR
        try:
            intercom.THREADS_DIR=self.root/'agent intercom/communication-threads'
            self.write('agent intercom/communication-threads/00_THREAD_INDEX.md','# Index\n')
            self.assertEqual([p.name for p in intercom.thread_files()],['original.md'])
        finally:intercom.THREADS_DIR=original


if __name__=='__main__': unittest.main()
