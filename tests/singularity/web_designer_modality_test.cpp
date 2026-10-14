#include "Singularity/Foreign/Web/Designer/WebConcept.hpp"
#include "Singularity/Foreign/Web/Designer/WebDesignerMetalaws.hpp"
#include "Singularity/Foreign/Web/Designer/WebRealizer.hpp"
#include "Singularity/Foreign/Web/Designer/WebDesignerChannel.hpp"
#include "Singularity/Storage/VirtualFileSystem.hpp"
#include "Singularity/Storage/FileChannel.hpp"
#include "ZonesOfEarth/AuthorsOfLaw/Law.hpp"
#include "ConstructedBeing/Singular/Object/Object.hpp"
#include "ConstructedBeing/Singular/Lexeme/Lexeme.hpp"
#include "Person/Person.hpp"
#include "Singularity/Core/EventBus.hpp"

#include <cassert>
#include <iostream>
#include <filesystem>
#include <vector>

using namespace Singularity::Foreign::Web;

static Person makeTestPerson(const std::string& name = "Zach") {
    Soul soul(name);
    Body body("humanoid", "default");
    Person p(std::move(soul), std::move(body), "default");
    p.setDisplayName(name);
    return p;
}

static Person& getTestPerson() {
    static Person s_person = makeTestPerson("Zach");
    return s_person;
}

void test_lexeme_resolution() {
    std::cout << "[Test 1] Lexeme & Relation In-World Resolution..." << std::endl;

    LawManager laws;
    // In accordance with Refusal 5 and Sun's instruction:
    // Built-in substrate vocabulary takes NO author parameter. Runtime initializer is not human author.
    WebDesignerMetalaws::syncRegister(laws);

    // Verify seeded in-world Lexemes
    const auto& lexemes = WebDesignerMetalaws::seededLexemes();
    assert(!lexemes.empty());

    // Verify seeded "denotes" Relations linking Lexemes to Laws
    const auto& relations = WebDesignerMetalaws::seededRelations();
    assert(!relations.empty());
    assert(relations.size() == lexemes.size());

    // Find heading Lexeme and its relation
    bool foundHeading = false;
    for (const auto& rel : relations) {
        assert(rel != nullptr);
        assert(rel->type == "denotes");
        const auto* lex = dynamic_cast<const Singularity::Language::Lexeme*>(rel->a());
        if (lex && lex->getSymbol() == "heading") {
            foundHeading = true;
            assert(rel->b() != nullptr);
            assert(rel->b()->getIdentifier() == "law.concept.role.heading");
            // Built-in substrate laws must NOT have human authors attached
            const auto* law = dynamic_cast<const Law*>(rel->b());
            assert(law != nullptr);
            assert(!law->isAuthored());
            assert(law->authors().getMembers().empty());
        }
    }
    assert(foundHeading);

    std::cout << "  ✓ All Lexemes, Laws, and denotes Relations seeded and linked correctly" << std::endl;
    std::cout << "  ✓ Built-in substrate vocabulary is unauthored infrastructure (no fake author)" << std::endl;
}

void test_metalaw_composition() {
    std::cout << "[Test 2] Law & Lexeme Universal Composition..." << std::endl;

    LawManager laws;
    Person& author = getTestPerson();
    Object channelTarget;
    channelTarget.setObjectID("test-channel-target");

    WebDesignerMetalaws::syncRegister(laws);

    // 1. "a large centered heading"
    {
        std::string err;
        WebConcept c = WebDesignerMetalaws::compilePhrase(
            "a large centered heading", laws, channelTarget, &err);
        assert(err.empty());
        assert(c.role == SemanticRole::Heading);
        assert(c.scale == ContentScale::Large);
        assert(c.align == LayoutAlign::Center);
    }

    // 2. "button rounded primary"
    {
        std::string err;
        WebConcept c = WebDesignerMetalaws::compilePhrase(
            "button rounded primary", laws, channelTarget, &err);
        assert(err.empty());
        assert(c.role == SemanticRole::Action);
        assert(c.shape == SurfaceShape::Rounded);
        assert(c.variant == SurfaceVariant::Primary);
    }

    // 3. "section vertical generous"
    {
        std::string err;
        WebConcept c = WebDesignerMetalaws::compilePhrase(
            "section vertical generous", laws, channelTarget, &err);
        assert(err.empty());
        assert(c.role == SemanticRole::Section);
        assert(c.flow == LayoutFlow::Vertical);
        assert(c.spacing == SpacingScale::Generous);
    }

    // 4. "card container subtle"
    {
        std::string err;
        WebConcept c = WebDesignerMetalaws::compilePhrase(
            "card container subtle", laws, channelTarget, &err);
        assert(err.empty());
        assert(c.role == SemanticRole::Container);
        assert(c.variant == SurfaceVariant::Card);
        assert(c.elevation == SurfaceElevation::Subtle);
    }

    std::cout << "  ✓ Universal concepts composed orthogonally onto target Singular" << std::endl;
}

void test_relational_ordering_and_provenance() {
    std::cout << "[Test 3] Relational Ordering & Provenance DAG..." << std::endl;

    LawManager laws;
    Person& author = getTestPerson();
    WebDesignerChannel channel;
    WebDesignerMetalaws::syncRegister(laws);

    // Create container
    auto grid = channel.authorElementFromPhrase("items-grid", "container grid spacing normal", "", &author, &laws);
    assert(grid != nullptr);

    // Create 3 sibling cards
    auto cardA = channel.authorElementFromPhrase("card-a", "card rounded subtle", "Alpha Item", &author, &laws);
    auto cardB = channel.authorElementFromPhrase("card-b", "card rounded subtle", "Beta Item", &author, &laws);
    auto cardC = channel.authorElementFromPhrase("card-c", "card rounded subtle", "Gamma Item", &author, &laws);

    // 1. Verify Real Earthcall Authorship Provenance Relation (Mandate 6)
    // The author endpoint MUST be the actual live Person, NEVER a Zach Plush!
    bool foundAuthRelA = false;
    for (const auto& rel : channel.getRelations()) {
        if (rel->type == WebRelationType::kAuthoredBy && rel->a() == cardA.get() && rel->b() == &author) {
            foundAuthRelA = true;
            assert(dynamic_cast<Person*>(rel->b()) != nullptr);
            assert(static_cast<Person*>(rel->b())->getDisplayName() == "Zach");
            break;
        }
    }
    assert(foundAuthRelA);

    // 2. Add containment relations (arbitrary order)
    channel.relateContains(grid, cardA);
    channel.relateContains(grid, cardB);
    channel.relateContains(grid, cardC);

    // 3. Assert Topological Ordering via "precedes" relations (Mandate 7):
    // Desired sequence: Card C precedes Card A, Card A precedes Card B (C -> A -> B)
    channel.relatePrecedes(cardC, cardA);
    channel.relatePrecedes(cardA, cardB);

    // Realize the grid element
    std::string html = WebRealizer::realizeElement(*grid, channel.getRelations(), 0);

    size_t posC = html.find("id=\"card-c\"");
    size_t posA = html.find("id=\"card-a\"");
    size_t posB = html.find("id=\"card-b\"");

    assert(posC != std::string::npos);
    assert(posA != std::string::npos);
    assert(posB != std::string::npos);

    // Precedence topological order MUST hold: C < A < B
    assert(posC < posA);
    assert(posA < posB);

    std::cout << "  ✓ Sibling sequence derived strictly from precedes Relation topology" << std::endl;
    std::cout << "  ✓ Provenance relation authored-by verified on authored element" << std::endl;
}

void test_thin_realizer_and_authored_theme() {
    std::cout << "[Test 4] Thin WebRealizer & Authored Theme Tokens..." << std::endl;

    LawManager laws;
    Person& author = getTestPerson();
    WebDesignerChannel channel;
    WebDesignerMetalaws::syncRegister(laws);

    auto page = channel.authorElementFromPhrase("themed-page", "page vertical", "", &author, &laws);

    // Author theme tokens on root Singular (Mandate 8)
    page->setDynamicProperty("theme.primaryColor", PropertyValue(std::string("#10b981")));
    page->setDynamicProperty("theme.backgroundColor", PropertyValue(std::string("#020617")));
    page->setDynamicProperty("theme.radius", PropertyValue(std::string("1.25rem")));

    RealizeOptions opts;
    opts.title = "Themed Page";
    std::string html = channel.realize(*page, opts);

    // Verify authored design tokens appear in CSS variables
    assert(html.find("--web-primary: #10b981;") != std::string::npos);
    assert(html.find("--web-bg: #020617;") != std::string::npos);
    assert(html.find("--web-radius: 1.25rem;") != std::string::npos);

    // Verify Mandate 11: NO unauthored #earthcall-feedback toast in CSS or JS!
    assert(html.find("#earthcall-feedback") == std::string::npos);
    assert(html.find("showFeedback") == std::string::npos);

    std::cout << "  ✓ Authored theme tokens emitted into :root CSS" << std::endl;
    std::cout << "  ✓ Unauthored feedback toast substrate completely removed" << std::endl;
}

void test_extensibility_exit_test() {
    std::cout << "[Test 5] Extensibility Exit Test: Zero C++ Enum Modification..." << std::endl;

    LawManager laws;
    Person& author = getTestPerson();
    Object target;
    target.setObjectID("target-sanctuary");

    // Introduce a brand-new semantic concept: "sanctuary" as role, and "ethereal" as surface variant.
    // NEITHER "sanctuary" nor "ethereal" exists in any C++ enum!
    auto lexSanctuary = std::make_shared<Singularity::Language::Lexeme>("sanctuary", "lexeme.custom.sanctuary");
    auto lawSanctuary = std::make_shared<Law>("law.custom.sanctuary", std::vector<Singular*>{&author});
    lawSanctuary->setActionModel(ActionNode::sequence({
        ActionNode::set("role", PropertyValue(std::string("sanctuary"))),
        ActionNode::set("surface.variant", PropertyValue(std::string("ethereal"))),
        ActionNode::set("prominence", PropertyValue(std::string("luminous")))
    }));
    laws.add(lawSanctuary);

    // Connect Lexeme <---denotes---> Law
    auto rel = std::make_shared<Relation>("denotes", *lexSanctuary, *lawSanctuary, true, 1.0f);

    // Compile phrase incorporating the custom concept and universal modifiers
    std::string err;
    WebConcept compiled = WebDesignerMetalaws::compilePhrase(
        "a sanctuary centered generous", laws, target, &err, {rel.get()});

    assert(err.empty());
    assert(compiled.roleName == "sanctuary");
    assert(compiled.variantName == "ethereal");
    assert(compiled.prominence == "luminous");
    assert(compiled.align == LayoutAlign::Center);
    assert(compiled.spacing == SpacingScale::Generous);

    // Realize element to HTML
    auto elem = WebDesignerMetalaws::createAuthoredElement("custom-sanctuary-1", compiled, &author);
    std::string html = WebRealizer::realizeElement(*elem, {});

    // Verify open class names and attributes rendered faithfully
    assert(html.find("class=\"web-sanctuary") != std::string::npos);
    assert(html.find("web-surface--ethereal") != std::string::npos);
    assert(html.find("web-prominence--luminous") != std::string::npos);
    assert(html.find("data-being-id=\"custom-sanctuary-1\"") != std::string::npos);

    std::cout << "  ✓ Brand new semantic concept authored purely via in-world Lexeme & Law" << std::endl;
    std::cout << "  ✓ Zero C++ enum modifications required for new concept" << std::endl;
}

void test_vfs_and_file_channel_storage() {
    std::cout << "[Test 6] Orthogonal Storage via VFS & FileChannel..." << std::endl;

    LawManager laws;
    Person& author = getTestPerson();
    WebDesignerChannel channel;
    WebDesignerMetalaws::syncRegister(laws);

    auto page = channel.authorElementFromPhrase("vfs-page", "page vertical", "", &author, &laws);
    auto heading = channel.authorElementFromPhrase("vfs-title", "heading large bold centered", "VFS Storage Test", &author, &laws);
    channel.relateContains(page, heading);

    // 1. Save to in-memory VFS URI (zero disk I/O)
    std::string memUri = "memory://web_designer/test_site.html";
    std::string html = channel.realize(*page);
    bool memOk = Singularity::Storage::VirtualFileSystem::instance().writeMemoryFile(memUri, html);
    assert(memOk);
    assert(Singularity::Storage::VirtualFileSystem::instance().memoryFileExists(memUri));

    std::string memContent;
    bool readOk = Singularity::Storage::VirtualFileSystem::instance().readMemoryFile(memUri, memContent);
    assert(readOk);
    assert(memContent.find("VFS Storage Test") != std::string::npos);

    // 2. Decoupled Storage: Save to persistent physical path via FileChannel directly
    std::string filePath = "saves/web_designer/test_governed_vfs.html";
    Singularity::Storage::FileChannel fc;
    PropertyPath::parse("file.path").setValue(fc, PropertyValue(filePath));
    PropertyPath::parse("file.content").setValue(fc, PropertyValue(html));
    PropertyPath::parse("file.atomicWrite").setValue(fc, PropertyValue(true));
    bool fcOk = fc.executeWrite();
    assert(fcOk);
    assert(std::filesystem::exists(filePath));
    assert(std::filesystem::file_size(filePath) > 200);
    std::filesystem::remove(filePath);

    // 3. Also verify saveToVfs convenience method on channel
    bool saveOk = channel.saveToVfs(*page, memUri, &laws);
    assert(saveOk);

    std::cout << "  ✓ In-memory VFS (memory://) write verified without disk I/O" << std::endl;
    std::cout << "  ✓ Decoupled FileChannel storage verified without raw std::ofstream" << std::endl;
}

void test_law_driven_behavior() {
    std::cout << "[Test 7] Law-Driven Event Reactivity..." << std::endl;

    LawManager laws;
    Person& author = getTestPerson();
    WebDesignerChannel channel;
    WebDesignerMetalaws::syncRegister(laws);
    laws.connectToEventBus();

    auto page = channel.authorElementFromPhrase("page-root", "page vertical", "", &author, &laws);
    auto btn = channel.authorElementFromPhrase("theme-btn", "button rounded primary", "Toggle Theme", &author, &laws);
    btn->setDynamicProperty("web.action.event", PropertyValue(std::string("theme-toggled")));

    channel.relateContains(page, btn, 1);

    // Initial state
    std::string btnMarkupInitial = WebRealizer::realizeElement(*btn, channel.getRelations(), 0);
    assert(btnMarkupInitial.find("web-surface--primary") != std::string::npos);

    // Register a dynamic law that reacts to "theme-toggled"
    auto themeLaw = std::make_shared<Law>("law-theme-switcher", std::vector<Singular*>{&author});
    themeLaw->setLawIdentifier("law-theme-switcher");
    themeLaw->grantAuthority(5);
    themeLaw->setActivation(Law::Activation::OnEvent);
    themeLaw->setScope(Law::Scope::Subject);
    themeLaw->addTarget(*btn);
    themeLaw->setActionModel(ActionNode::sequence({
        ActionNode::set("surface.variant", PropertyValue(std::string("secondary"))),
        ActionNode::set("web.surface.variant", PropertyValue(std::string("secondary")))
    }));
    laws.add(themeLaw);
    laws.bindTrigger("law-theme-switcher", "theme-toggled");

    // Ingest the web action
    channel.handleWebAction("theme-btn", "theme-toggled", laws, btn.get());

    // Verify property mutated by Earthcall Law
    PropertyValue varVal;
    assert(btn->getDynamicProperty("surface.variant", varVal));
    assert(std::get<std::string>(varVal) == "secondary");

    // Re-realize: new markup reflects the Law's action
    std::string btnMarkupUpdated = WebRealizer::realizeElement(*btn, channel.getRelations(), 0);
    assert(btnMarkupUpdated.find("web-surface--secondary") != std::string::npos);

    std::cout << "  ✓ Web actions dispatched to EventBus, triggered Laws, and re-realized truthfully" << std::endl;
}

void test_end_to_end_prototype_generation() {
    std::cout << "[Test 8] End-to-End Frontend Website Prototype Generation..." << std::endl;

    LawManager laws;
    Person& author = getTestPerson();
    WebDesignerChannel channel;
    WebDesignerMetalaws::syncRegister(laws);

    // 1. Root Page
    auto page = channel.authorElementFromPhrase("earthcall-prototype", "page vertical", "", &author, &laws);
    page->setDynamicProperty("theme.primaryColor", PropertyValue(std::string("#38bdf8")));
    page->setDynamicProperty("theme.backgroundColor", PropertyValue(std::string("#0b0f19")));
    page->setDynamicProperty("theme.radius", PropertyValue(std::string("0.75rem")));

    // 2. Navigation Landmark
    auto nav = channel.authorElementFromPhrase("navbar", "navigation horizontal between align center", "", &author, &laws);
    auto brand = channel.authorElementFromPhrase("nav-brand", "heading normal bold", "Earthcall Ourverse", &author, &laws);
    auto navLinks = channel.authorElementFromPhrase("nav-links", "container horizontal spacing relaxed align center", "", &author, &laws);
    auto l1 = channel.authorElementFromPhrase("nav-link-manifesto", "link normal", "Manifesto", &author, &laws);
    l1->setDynamicProperty("action.href", PropertyValue(std::string("#manifesto")));
    auto l2 = channel.authorElementFromPhrase("nav-link-ontology", "link normal", "Ontology", &author, &laws);
    l2->setDynamicProperty("action.href", PropertyValue(std::string("#ontology")));
    auto l3 = channel.authorElementFromPhrase("nav-link-laws", "link normal", "Living Laws", &author, &laws);
    l3->setDynamicProperty("action.href", PropertyValue(std::string("#laws")));
    auto navCta = channel.authorElementFromPhrase("nav-cta", "button rounded primary", "Enter Ourverse", &author, &laws);
    navCta->setDynamicProperty("action.event", PropertyValue(std::string("enter-ourverse")));

    channel.relateContains(navLinks, l1);
    channel.relateContains(navLinks, l2);
    channel.relateContains(navLinks, l3);
    channel.relatePrecedes(l1, l2);
    channel.relatePrecedes(l2, l3);

    channel.relateContains(nav, brand);
    channel.relateContains(nav, navLinks);
    channel.relateContains(nav, navCta);
    channel.relatePrecedes(brand, navLinks);
    channel.relatePrecedes(navLinks, navCta);

    // 3. Hero Section
    auto hero = channel.authorElementFromPhrase("hero-section", "section vertical centered generous hero", "", &author, &laws);
    auto heroTitle = channel.authorElementFromPhrase("hero-title", "heading huge bold centered", "Earthcall Ourverse", &author, &laws);
    auto heroLead = channel.authorElementFromPhrase("hero-lead", "paragraph large lead centered",
        "A Person-centered ontology that orders the engine attached to it. A vessel of shared human life that glorifies God and holds encounters with Christ and each other.", &author, &laws);
    auto heroActions = channel.authorElementFromPhrase("hero-actions", "container horizontal centered spacing normal", "", &author, &laws);
    auto ctaPrimary = channel.authorElementFromPhrase("hero-cta-primary", "button rounded primary large", "Explore Living Laws", &author, &laws);
    ctaPrimary->setDynamicProperty("action.event", PropertyValue(std::string("explore-laws-clicked")));
    auto ctaSecondary = channel.authorElementFromPhrase("hero-cta-secondary", "button rounded outline large", "Read The Manifesto", &author, &laws);
    ctaSecondary->setDynamicProperty("action.href", PropertyValue(std::string("#manifesto")));

    channel.relateContains(heroActions, ctaPrimary);
    channel.relateContains(heroActions, ctaSecondary);
    channel.relatePrecedes(ctaPrimary, ctaSecondary);

    channel.relateContains(hero, heroTitle);
    channel.relateContains(hero, heroLead);
    channel.relateContains(hero, heroActions);
    channel.relatePrecedes(heroTitle, heroLead);
    channel.relatePrecedes(heroLead, heroActions);

    // 4. Core Pillars / Features Grid
    auto featuresSec = channel.authorElementFromPhrase("features-section", "section vertical generous", "", &author, &laws);
    featuresSec->setDynamicProperty("id", PropertyValue(std::string("ontology")));
    auto featTitle = channel.authorElementFromPhrase("features-title", "heading large bold centered", "Architectural Pillars", &author, &laws);
    auto featSub = channel.authorElementFromPhrase("features-subtitle", "paragraph normal muted centered",
        "The minimum viable invariants enabling maximum generative, logical, and teleological expressibility.", &author, &laws);
    auto featGrid = channel.authorElementFromPhrase("features-grid", "container grid spacing generous", "", &author, &laws);

    // Card 1
    auto c1 = channel.authorElementFromPhrase("card-person", "card container rounded subtle", "", &author, &laws);
    auto c1Title = channel.authorElementFromPhrase("card-person-title", "heading normal bold", "Person-Centered Ontology", &author, &laws);
    auto c1Text = channel.authorElementFromPhrase("card-person-text", "paragraph normal muted",
        "No C++ class for a domain noun. A Person means an actual human being. Machines and models sense and act; Persons author what things ARE.", &author, &laws);
    auto c1Btn = channel.authorElementFromPhrase("card-person-btn", "link normal", "Learn more →", &author, &laws);
    channel.relateContains(c1, c1Title);
    channel.relateContains(c1, c1Text);
    channel.relateContains(c1, c1Btn);
    channel.relatePrecedes(c1Title, c1Text);
    channel.relatePrecedes(c1Text, c1Btn);

    // Card 2
    auto c2 = channel.authorElementFromPhrase("card-laws", "card container rounded subtle", "", &author, &laws);
    auto c2Title = channel.authorElementFromPhrase("card-laws-title", "heading normal bold", "Living Laws & Metalaws", &author, &laws);
    auto c2Text = channel.authorElementFromPhrase("card-laws-text", "paragraph normal muted",
        "Behavior emerges from Person-authored Laws, not hardcoded algorithms. Metalaws govern and compile authored sentences without a black box.", &author, &laws);
    auto c2Btn = channel.authorElementFromPhrase("card-laws-btn", "link normal", "View law engine →", &author, &laws);
    channel.relateContains(c2, c2Title);
    channel.relateContains(c2, c2Text);
    channel.relateContains(c2, c2Btn);
    channel.relatePrecedes(c2Title, c2Text);
    channel.relatePrecedes(c2Text, c2Btn);

    // Card 3
    auto c3 = channel.authorElementFromPhrase("card-ontomath", "card container rounded subtle", "", &author, &laws);
    auto c3Title = channel.authorElementFromPhrase("card-ontomath-title", "heading normal bold", "OntoMath Continuous Fields", &author, &laws);
    auto c3Text = channel.authorElementFromPhrase("card-ontomath-text", "paragraph normal muted",
        "Mathematical meaning owned in closed form: continuous procedural manifolds, signed distance fields, and exact analytic rendering.", &author, &laws);
    auto c3Btn = channel.authorElementFromPhrase("card-ontomath-btn", "link normal", "Explore geometry →", &author, &laws);
    channel.relateContains(c3, c3Title);
    channel.relateContains(c3, c3Text);
    channel.relateContains(c3, c3Btn);
    channel.relatePrecedes(c3Title, c3Text);
    channel.relatePrecedes(c3Text, c3Btn);

    channel.relateContains(featGrid, c1);
    channel.relateContains(featGrid, c2);
    channel.relateContains(featGrid, c3);
    channel.relatePrecedes(c1, c2);
    channel.relatePrecedes(c2, c3);

    channel.relateContains(featuresSec, featTitle);
    channel.relateContains(featuresSec, featSub);
    channel.relateContains(featuresSec, featGrid);
    channel.relatePrecedes(featTitle, featSub);
    channel.relatePrecedes(featSub, featGrid);

    // 5. Interactive Law Demonstration Section
    auto demoSec = channel.authorElementFromPhrase("demo-section", "section vertical centered generous", "", &author, &laws);
    demoSec->setDynamicProperty("id", PropertyValue(std::string("laws")));
    auto demoTitle = channel.authorElementFromPhrase("demo-title", "heading large bold centered", "Live Law Interactivity Witness", &author, &laws);
    auto demoCard = channel.authorElementFromPhrase("demo-card", "card container rounded subtle", "", &author, &laws);
    auto demoText = channel.authorElementFromPhrase("demo-text", "paragraph normal centered",
        "Every element on this page was authored by Earthcall Laws and Lexemes. Click below to verify live EventBus event firing:", &author, &laws);
    auto demoBtn = channel.authorElementFromPhrase("demo-btn", "button rounded primary", "Invoke Law Event (Test)", &author, &laws);
    demoBtn->setDynamicProperty("action.event", PropertyValue(std::string("interactive-witness-clicked")));
    channel.relateContains(demoCard, demoText);
    channel.relateContains(demoCard, demoBtn);
    channel.relatePrecedes(demoText, demoBtn);

    channel.relateContains(demoSec, demoTitle);
    channel.relateContains(demoSec, demoCard);
    channel.relatePrecedes(demoTitle, demoCard);

    // 6. Subordinated Notes Section (exercising 'subordinate' and 'quiet')
    auto notesSec = channel.authorElementFromPhrase("notes-section", "section vertical centered relaxed", "", &author, &laws);
    auto noteText = channel.authorElementFromPhrase("subordinate-note", "paragraph small quiet centered subordinate",
        "Note: This website is rendered as an ephemeral realization target of living Earthcall world state.", &author, &laws);
    channel.relateContains(notesSec, noteText);

    // 7. Manifesto Quote Section
    auto quoteSec = channel.authorElementFromPhrase("quote-section", "section vertical centered relaxed", "", &author, &laws);
    quoteSec->setDynamicProperty("id", PropertyValue(std::string("manifesto")));
    auto quote = channel.authorElementFromPhrase("manifesto-quote", "quote centered prominent",
        "\"Earthcall is a Person-centered ontology that orders the engine attached to it. The engine here is not the order of truth in Earthcall—the ontology is, and the engine serves as the vessel for that.\" — Zachary Zhang", &author, &laws);
    channel.relateContains(quoteSec, quote);

    // 8. Footer Section
    auto footer = channel.authorElementFromPhrase("footer-section", "section horizontal between align center", "", &author, &laws);
    auto footerText = channel.authorElementFromPhrase("footer-text", "paragraph small muted",
        "© 2026 Earthcall Ourverse. Authored natively by Earthcall Laws, Lexemes, and Metalaws.", &author, &laws);
    auto footerLinks = channel.authorElementFromPhrase("footer-links", "container horizontal spacing normal", "", &author, &laws);
    auto fl1 = channel.authorElementFromPhrase("footer-link-1", "link small muted", "Seven Refusals", &author, &laws);
    auto fl2 = channel.authorElementFromPhrase("footer-link-2", "link small muted", "Agent Compass", &author, &laws);
    channel.relateContains(footerLinks, fl1);
    channel.relateContains(footerLinks, fl2);
    channel.relatePrecedes(fl1, fl2);
    channel.relateContains(footer, footerText);
    channel.relateContains(footer, footerLinks);
    channel.relatePrecedes(footerText, footerLinks);

    // Relate all sections to page in topological sequence
    channel.relateContains(page, nav);
    channel.relateContains(page, hero);
    channel.relateContains(page, featuresSec);
    channel.relateContains(page, demoSec);
    channel.relateContains(page, notesSec);
    channel.relateContains(page, quoteSec);
    channel.relateContains(page, footer);

    channel.relatePrecedes(nav, hero);
    channel.relatePrecedes(hero, featuresSec);
    channel.relatePrecedes(featuresSec, demoSec);
    channel.relatePrecedes(demoSec, notesSec);
    channel.relatePrecedes(notesSec, quoteSec);
    channel.relatePrecedes(quoteSec, footer);

    // Realize and export via VFS
    RealizeOptions opts;
    opts.title = "Earthcall Ourverse — Law-Authored Web Designer Prototype";
    opts.metaDescription = "A genuine frontend prototype authored entirely by Earthcall Laws and Lexemes.";

    bool ok1 = channel.saveToVfs(*page, "web_ui/designer_prototype.html", &laws, opts);
    assert(ok1);
    bool ok2 = channel.saveToVfs(*page, "saves/web_designer/prototype.html", &laws, opts);
    assert(ok2);

    // Verify files exist and are substantial
    assert(std::filesystem::exists("web_ui/designer_prototype.html"));
    assert(std::filesystem::file_size("web_ui/designer_prototype.html") > 5000);
    assert(std::filesystem::exists("saves/web_designer/prototype.html"));

    std::cout << "  ✓ Exported complete working frontend prototype to web_ui/designer_prototype.html ("
              << std::filesystem::file_size("web_ui/designer_prototype.html") << " bytes)" << std::endl;
}

void test_alice_and_bob_two_person_provenance() {
    std::cout << "[Test 9] Multi-Person Authorship Provenance Isolation (Alice & Bob)..." << std::endl;

    // 1. Two distinct live Persons
    Person alice = makeTestPerson("Alice");
    Person bob = makeTestPerson("Bob");

    // 2. Alice initializes Web modality in her context
    LawManager lawsAlice;
    WebDesignerMetalaws::syncRegister(lawsAlice);

    // 3. Bob initializes Web modality in his context
    LawManager lawsBob;
    WebDesignerMetalaws::syncRegister(lawsBob);

    // ASSERT Criterion 3: Built-in substrate vocabulary does not become Alice-authored or Bob-authored
    Law* headingLawAlice = lawsAlice.find("law.concept.role.heading");
    assert(headingLawAlice != nullptr);
    assert(!headingLawAlice->isAuthored());
    assert(headingLawAlice->authors().getMembers().empty());

    Law* headingLawBob = lawsBob.find("law.concept.role.heading");
    assert(headingLawBob != nullptr);
    assert(!headingLawBob->isAuthored());
    assert(headingLawBob->authors().getMembers().empty());

    // 4. Alice authors a distinct custom law
    auto lawAlice = std::make_shared<Law>("law.alice.brand", std::vector<Singular*>{&alice});
    lawAlice->setActionModel(ActionNode::set("role", PropertyValue(std::string("alice-brand"))));
    lawsAlice.add(lawAlice);

    // 5. Bob authors a distinct custom law
    auto lawBob = std::make_shared<Law>("law.bob.brand", std::vector<Singular*>{&bob});
    lawBob->setActionModel(ActionNode::set("role", PropertyValue(std::string("bob-brand"))));
    lawsBob.add(lawBob);

    assert(lawAlice->isAuthored());
    assert(lawAlice->authors().getMembers().size() == 1);
    assert(lawAlice->authors().getMembers()[0] == &alice);

    assert(lawBob->isAuthored());
    assert(lawBob->authors().getMembers().size() == 1);
    assert(lawBob->authors().getMembers()[0] == &bob);

    // 6. Alice and Bob author web elements through channels
    WebDesignerChannel channelAlice;
    auto elemAlice = channelAlice.authorElementFromPhrase("alice-box", "container rounded", "", &alice, &lawsAlice);

    WebDesignerChannel channelBob;
    auto elemBob = channelBob.authorElementFromPhrase("bob-box", "container rounded", "", &bob, &lawsBob);

    // ASSERT Criterion 1 & 5: Authorship is strictly Person-exclusive and independent
    bool foundAliceAuth = false;
    for (const auto& rel : channelAlice.getRelations()) {
        if (rel->type == WebRelationType::kAuthoredBy && rel->a() == elemAlice.get()) {
            assert(rel->b() == &alice);
            assert(dynamic_cast<Person*>(rel->b()) != nullptr);
            foundAliceAuth = true;
        }
    }
    assert(foundAliceAuth);

    bool foundBobAuth = false;
    for (const auto& rel : channelBob.getRelations()) {
        if (rel->type == WebRelationType::kAuthoredBy && rel->a() == elemBob.get()) {
            assert(rel->b() == &bob);
            assert(dynamic_cast<Person*>(rel->b()) != nullptr);
            assert(rel->b() != &alice); // Bob does NOT inherit Alice as author
            foundBobAuth = true;
        }
    }
    assert(foundBobAuth);

    std::cout << "  ✓ Built-in substrate vocabulary remained unauthored across both initializations" << std::endl;
    std::cout << "  ✓ Alice-authored state and Bob-authored state maintained strictly distinct Person provenance" << std::endl;
    std::cout << "  ✓ Bob did not inherit Alice as author (no process-global leak)" << std::endl;
}

void test_person_absence_and_enduring_authorship() {
    std::cout << "[Test 10] Person Absence & Enduring Authorship..." << std::endl;

    LawManager laws;
    WebDesignerMetalaws::syncRegister(laws);

    // Person Carol authored a Law earlier
    auto carol = std::make_shared<Person>(makeTestPerson("Carol"));
    auto enduringLaw = std::make_shared<Law>("law.carol.accent", std::vector<Singular*>{carol.get()});
    enduringLaw->setActionModel(ActionNode::set("surface.variant", PropertyValue(std::string("accent"))));
    laws.add(enduringLaw);

    assert(enduringLaw->isAuthored());
    assert(enduringLaw->authors().getMembers()[0] == carol.get());

    // Carol is absent now: no Person is passed to the execution environment or runtime call.
    // In accordance with Sun's instruction:
    // "Person authored Law earlier -> authorship persists -> Person may be absent now -> Law still exists and executes"
    // No cube or Object proxy needed!
    assert(enduringLaw->isAuthored());

    Object testTarget;
    testTarget.setObjectID("absence-test-target");
    testTarget.setDynamicProperty("surface.variant", PropertyValue(std::string("default")));

    // Law applies cleanly without Carol being present at runtime
    auto result = enduringLaw->applyTo(testTarget);
    assert(result == Law::ApplicationResult::Applied);

    PropertyValue v;
    assert(testTarget.getDynamicProperty("surface.variant", v));
    assert(std::get<std::string>(v) == "accent");

    std::cout << "  ✓ Enduring authorship persists across Person absence" << std::endl;
    std::cout << "  ✓ Law executed truthfully without proxy objects or cubes babysitting it" << std::endl;
}

void test_browser_trust_boundary_security() {
    std::cout << "[Test 11] Browser Trust Boundary Security..." << std::endl;

    // 1. HTML Tag Sanitization
    assert(WebRealizer::sanitizeHtmlTag("div") == "div");
    assert(WebRealizer::sanitizeHtmlTag("section") == "section");
    assert(WebRealizer::sanitizeHtmlTag("custom-card") == "custom-card");
    assert(WebRealizer::sanitizeHtmlTag("script") == "div");
    assert(WebRealizer::sanitizeHtmlTag("style") == "div");
    assert(WebRealizer::sanitizeHtmlTag("iframe") == "div");
    assert(WebRealizer::sanitizeHtmlTag("<script>alert(1)</script>") == "div");
    assert(WebRealizer::sanitizeHtmlTag("123bad") == "div");
    assert(WebRealizer::sanitizeHtmlTag("") == "div");

    // 2. CSS Token Normalization
    assert(WebRealizer::normalizeCssToken("heading") == "heading");
    assert(WebRealizer::normalizeCssToken("luminous glow") == "luminous-glow");
    assert(WebRealizer::normalizeCssToken("bad!class@name#") == "bad-class-name");
    assert(WebRealizer::normalizeCssToken("___foo---bar___") == "foo-bar");

    // 3. URL Scheme Policy on Hrefs
    assert(WebRealizer::sanitizeHref("https://earthcall.org") == "https://earthcall.org");
    assert(WebRealizer::sanitizeHref("http://example.com/demo") == "http://example.com/demo");
    assert(WebRealizer::sanitizeHref("/docs/manifesto") == "/docs/manifesto");
    assert(WebRealizer::sanitizeHref("#section-1") == "#section-1");
    assert(WebRealizer::sanitizeHref("mailto:contact@earthcall.org") == "mailto:contact@earthcall.org");
    assert(WebRealizer::sanitizeHref("tel:+1234567890") == "tel:+1234567890");

    // Rejected executable schemes
    assert(WebRealizer::sanitizeHref("javascript:alert(1)") == "#");
    assert(WebRealizer::sanitizeHref("JAVASCRIPT:evil()") == "#");
    assert(WebRealizer::sanitizeHref("data:text/html,<script>evil()</script>") == "#");
    assert(WebRealizer::sanitizeHref("vbscript:msgbox") == "#");

    // Adversarial: leading control characters, tabs, spaces, and bypass attempts
    assert(WebRealizer::sanitizeHref("\x01\x02javascript:alert(1)") == "#");
    assert(WebRealizer::sanitizeHref("\t\r\n  JAVASCRIPT:attack()") == "#");
    assert(WebRealizer::sanitizeHref("java\tscript:alert(1)") == "#");
    assert(WebRealizer::sanitizeHref("data:text/html,<script>evil()</script>") == "#");
    assert(WebRealizer::sanitizeHref("DATA:image/svg+xml,...") == "#");
    assert(WebRealizer::sanitizeHref("file:///etc/passwd") == "#");
    assert(WebRealizer::sanitizeHref("blob:http://example.com/uuid") == "#");

    // 4. CSS Value </style> Breakout Neutralization
    std::string maliciousCss = "red; </style><script>alert('xss')</script><style>";
    std::string safeCss = WebRealizer::sanitizeCssValue(maliciousCss);
    assert(safeCss.find("</style") == std::string::npos);
    assert(safeCss.find("<\\/style") != std::string::npos);

    // Adversarial: spaces inside tag, HTML comments
    std::string spacedCss = "blue; < / style > <script>alert(1)</script>";
    std::string safeSpacedCss = WebRealizer::sanitizeCssValue(spacedCss);
    assert(safeSpacedCss.find("</style") == std::string::npos);
    assert(safeSpacedCss.find("<\\/") != std::string::npos);

    std::string commentCss = "green; <!-- </style> -->";
    std::string safeCommentCss = WebRealizer::sanitizeCssValue(commentCss);
    assert(safeCommentCss.find("<!--") == std::string::npos);
    assert(safeCommentCss.find("-->") == std::string::npos);

    // Verify in generateStyleSheet with malicious authored root property
    Object maliciousRoot;
    maliciousRoot.setObjectID("malicious-root");
    maliciousRoot.setDynamicProperty("theme.backgroundColor", PropertyValue(maliciousCss));

    std::string sheet = WebRealizer::generateStyleSheet(&maliciousRoot);
    assert(sheet.find("</style") == std::string::npos);

    std::cout << "  ✓ HTML tag validation blocks script/iframe/malformed tags" << std::endl;
    std::cout << "  ✓ CSS token normalization sanitizes arbitrary class tokens" << std::endl;
    std::cout << "  ✓ Href policy blocks javascript: and data: schemes" << std::endl;
    std::cout << "  ✓ CSS value sanitization neutralizes </style> breakout" << std::endl;
}

void test_universe_discovery_and_pure_realization() {
    std::cout << "[Test 12] Universe Discovery & Pure Realization..." << std::endl;

    Person& author = getTestPerson();
    WebConcept cardConcept;
    cardConcept.roleName = "card";
    cardConcept.text = "In-World Discovered Card";

    auto inWorldElement = WebDesignerMetalaws::createAuthoredElement("universe-elem-42", cardConcept, &author);

    // Register with Universe
    std::vector<Singular*> worldBeings = { inWorldElement.get() };
    Universe::instance().setProvider([&](std::vector<Singular*>& out) {
        for (auto* b : worldBeings) out.push_back(b);
    });

    WebDesignerChannel channel;
    // Discover being through Universe even though channel never saw it
    auto found = channel.findElement("universe-elem-42");
    assert(found != nullptr);
    assert(found->getIdentifier() == "universe-elem-42");

    // Pure realization to HTML without touching storage or filesystem
    std::string html = WebRealizer::realizeElement(*found, {});
    assert(!html.empty());
    assert(html.find("In-World Discovered Card") != std::string::npos);
    assert(html.find("data-being-id=\"universe-elem-42\"") != std::string::npos);

    // Reset Universe provider
    Universe::instance().setProvider(nullptr);

    std::cout << "  ✓ Element discovered through ordinary Earthcall Universe world ownership" << std::endl;
    std::cout << "  ✓ Pure realization returns valid HTML without requiring storage" << std::endl;
}

void test_relational_precedence_overrides_numeric_order() {
    std::cout << "[Test 13] Relational Precedence Overrides Numeric Order..." << std::endl;

    Person& author = getTestPerson();
    WebDesignerChannel channel;

    // Create container
    auto container = channel.authorElementFromPhrase("conflict-container", "container", "", &author);

    // Create two children: childFirst and childSecond
    auto childFirst = channel.authorElementFromPhrase("elem-first", "item", "First Sibling", &author);
    auto childSecond = channel.authorElementFromPhrase("elem-second", "item", "Second Sibling", &author);

    // Relational topology: childFirst precedes childSecond
    channel.relateContains(container, childFirst);
    channel.relateContains(container, childSecond);
    channel.relatePrecedes(childFirst, childSecond);

    // Adversarial numeric fiat: childSecond has web.order = 1, childFirst has web.order = 100
    // In a flat numeric system, childSecond would render first.
    // In Earthcall, relational "precedes" DAG MUST strictly govern sequence!
    childSecond->setDynamicProperty("web.order", PropertyValue(1.0));
    childFirst->setDynamicProperty("web.order", PropertyValue(100.0));

    std::string html = channel.realize(*container);

    size_t posFirst = html.find("id=\"elem-first\"");
    size_t posSecond = html.find("id=\"elem-second\"");

    assert(posFirst != std::string::npos);
    assert(posSecond != std::string::npos);
    assert(posFirst < posSecond); // elem-first MUST appear before elem-second!

    std::cout << "  ✓ Directed 'precedes' Relation strictly governed sibling sequence" << std::endl;
    std::cout << "  ✓ Numeric web.order (1 vs 100) was refused as an override against relational DAG" << std::endl;
}

void test_channel_destruction_and_rediscovery_from_world() {
    std::cout << "[Test 14] Channel Destruction & World State Independence..." << std::endl;

    Person& author = getTestPerson();
    std::shared_ptr<Singular> rootElem;
    std::shared_ptr<Singular> childElem;
    std::shared_ptr<Relation> containsRel;

    // 1. Author site beings inside a scoped channel that will be destroyed
    {
        WebDesignerChannel temporaryChannel;
        rootElem = temporaryChannel.authorElementFromPhrase("site-root-destruct", "section", "", &author);
        childElem = temporaryChannel.authorElementFromPhrase("site-child-destruct", "paragraph", "Enduring World Content", &author);
        containsRel = temporaryChannel.relateContains(rootElem, childElem);
        // temporaryChannel goes out of scope and is destroyed here
    }

    // 2. Register beings and relations in ordinary Earthcall Universe / world state
    std::vector<Singular*> beings = { rootElem.get(), childElem.get() };
    std::vector<Relation*> rels = { containsRel.get() };

    Universe::instance().setProvider([&](std::vector<Singular*>& out) {
        for (auto* b : beings) out.push_back(b);
    });
    Universe::instance().setRelationProvider([&](std::vector<Relation*>& out) {
        for (auto* r : rels) out.push_back(r);
    });

    // 3. Create a brand new WebDesignerChannel that never saw the authoring calls
    WebDesignerChannel freshChannel;

    // Assert beings are discoverable from world state
    auto discoveredRoot = freshChannel.findElement("site-root-destruct");
    assert(discoveredRoot != nullptr);
    assert(discoveredRoot->getIdentifier() == "site-root-destruct");

    auto discoveredChild = freshChannel.findElement("site-child-destruct");
    assert(discoveredChild != nullptr);
    assert(discoveredChild->getIdentifier() == "site-child-destruct");

    // Realize entire site through the fresh channel
    std::string html = freshChannel.realize(*discoveredRoot);
    assert(!html.empty());
    assert(html.find("id=\"site-root-destruct\"") != std::string::npos);
    assert(html.find("id=\"site-child-destruct\"") != std::string::npos);
    assert(html.find("Enduring World Content") != std::string::npos);

    // Clean up Universe providers
    Universe::instance().setProvider(nullptr);
    Universe::instance().setRelationProvider(nullptr);

    std::cout << "  ✓ Authored site survived channel destruction completely" << std::endl;
    std::cout << "  ✓ Fresh channel discovered beings and realized site directly from world state" << std::endl;
}

int main() {
    std::cout << "========================================" << std::endl;
    std::cout << "Running Web Designer Modality Test Suite" << std::endl;
    std::cout << "========================================" << std::endl;

    test_lexeme_resolution();
    test_metalaw_composition();
    test_relational_ordering_and_provenance();
    test_thin_realizer_and_authored_theme();
    test_extensibility_exit_test();
    test_vfs_and_file_channel_storage();
    test_law_driven_behavior();
    test_end_to_end_prototype_generation();
    test_alice_and_bob_two_person_provenance();
    test_person_absence_and_enduring_authorship();
    test_browser_trust_boundary_security();
    test_universe_discovery_and_pure_realization();
    test_relational_precedence_overrides_numeric_order();
    test_channel_destruction_and_rediscovery_from_world();

    std::cout << "========================================" << std::endl;
    std::cout << "ALL WEB DESIGNER MODALITY TESTS PASSED!" << std::endl;
    std::cout << "========================================" << std::endl;
    return 0;
}
