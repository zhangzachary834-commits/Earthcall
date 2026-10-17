#include "Singularity/Foreign/Web/Designer/WebConcept.hpp"
#include "Singularity/Foreign/Web/Designer/WebDesignerMetalaws.hpp"
#include "Singularity/Foreign/Web/Designer/WebRealizer.hpp"
#include "Singularity/Foreign/Web/Designer/WebDesignerChannel.hpp"
#include "Singularity/Storage/VirtualFileSystem.hpp"
#include "Singularity/Storage/FileChannel.hpp"
#include "Singularity/Language/LanguageSystem.hpp"
#include "ZonesOfEarth/AuthorsOfLaw/Law.hpp"
#include "ZonesOfEarth/AuthorsOfLaw/Universe.hpp"
#include "ZonesOfEarth/Zone/Zone.hpp"
#include "Relation/Formation/Formation.hpp"
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

static void setupAuthoredVocabulary(Person& author, LawManager& laws, Formation& formation) {
    WebDesignerMetalaws::authorVocabulary(author, laws, &formation);
    Universe::instance().setRelationProvider([&formation](std::vector<Relation*>& out) {
        for (const auto& r : formation.relations().getAll()) {
            out.push_back(r.get());
        }
    });
}

static void teardownVocabulary() {
    Universe::instance().setRelationProvider(nullptr);
}

void test_lexeme_resolution() {
    std::cout << "[Test 1] Lexeme & Relation In-World Resolution..." << std::endl;

    LawManager laws;
    Person& author = getTestPerson();
    Formation formation;
    WebDesignerMetalaws::authorVocabulary(author, laws, &formation);

    // Verify in-world Lexemes and "denotes" Relations in formation
    const auto& relations = formation.relations().getAll();
    assert(!relations.empty());

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
            const auto* law = dynamic_cast<const Law*>(rel->b());
            assert(law != nullptr);
            assert(law->isAuthored());
            assert(law->authors().getMembers().size() == 1);
            assert(law->authors().getMembers()[0] == &author);
            // Sun Gate 10: Authored vocabulary laws receive ordinary authored authority (1), NOT kernel 10.
            assert(law->authorityLevel() == 1);
        }
    }
    assert(foundHeading);

    std::cout << "  ✓ All Lexemes, Laws, and denotes Relations authored and linked correctly" << std::endl;
    std::cout << "  ✓ Authored vocabulary has explicit Person stewardship (Zach)" << std::endl;
}

void test_metalaw_composition() {
    std::cout << "[Test 2] Law & Lexeme Universal Composition..." << std::endl;

    LawManager laws;
    Person& author = getTestPerson();
    Formation formation;
    setupAuthoredVocabulary(author, laws, formation);

    Object channelTarget;
    channelTarget.setObjectID("test-channel-target");

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

    teardownVocabulary();
    std::cout << "  ✓ Universal concepts composed orthogonally onto target Singular" << std::endl;
}

void test_relational_ordering_and_provenance() {
    std::cout << "[Test 3] Relational Ordering & Provenance DAG..." << std::endl;

    LawManager laws;
    Person& author = getTestPerson();
    Formation formation;
    setupAuthoredVocabulary(author, laws, formation);

    WebDesignerChannel channel;
    for (const auto& r : formation.relations().getAll()) {
        channel.addRelation(r);
    }

    // Create container
    auto grid = channel.authorElementFromPhrase("items-grid", "container grid spacing normal", "", &author, &laws);
    assert(grid != nullptr);

    // Create 3 sibling cards
    auto cardA = channel.authorElementFromPhrase("card-a", "card rounded subtle", "Alpha Item", &author, &laws);
    auto cardB = channel.authorElementFromPhrase("card-b", "card rounded subtle", "Beta Item", &author, &laws);
    auto cardC = channel.authorElementFromPhrase("card-c", "card rounded subtle", "Gamma Item", &author, &laws);

    // 1. Verify Real Earthcall Authorship Provenance Relation
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

    // 2. Add containment relations
    channel.relateContains(grid, cardA);
    channel.relateContains(grid, cardB);
    channel.relateContains(grid, cardC);

    // 3. Assert Topological Ordering via "precedes" relations: C -> A -> B
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

    teardownVocabulary();
    std::cout << "  ✓ Sibling sequence derived strictly from precedes Relation topology" << std::endl;
    std::cout << "  ✓ Provenance relation authored-by verified on authored element" << std::endl;
}

void test_thin_realizer_and_authored_theme() {
    std::cout << "[Test 4] Thin WebRealizer & Authored Theme Tokens..." << std::endl;

    LawManager laws;
    Person& author = getTestPerson();
    Formation formation;
    setupAuthoredVocabulary(author, laws, formation);

    WebDesignerChannel channel;
    for (const auto& r : formation.relations().getAll()) {
        channel.addRelation(r);
    }

    auto page = channel.authorElementFromPhrase("themed-page", "page vertical", "", &author, &laws);

    // Author theme tokens on root Singular
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

    // Verify NO unauthored feedback toast substrate
    assert(html.find("#earthcall-feedback") == std::string::npos);
    assert(html.find("showFeedback") == std::string::npos);

    teardownVocabulary();
    std::cout << "  ✓ Authored theme tokens emitted into :root CSS" << std::endl;
    std::cout << "  ✓ Unauthored feedback toast substrate completely removed" << std::endl;
}

void test_extensibility_exit_test() {
    std::cout << "[Test 5] Extensibility Exit Test: Zero C++ Enum Modification..." << std::endl;

    LawManager laws;
    Person& author = getTestPerson();
    Formation formation;
    setupAuthoredVocabulary(author, laws, formation);

    Object target;
    target.setObjectID("target-sanctuary");

    // Introduce a brand-new semantic concept: "sanctuary" as role, and "ethereal" as surface variant.
    // NEITHER "sanctuary" nor "ethereal" exists in any C++ enum!
    auto lexSanctuary = Singularity::Language::LanguageSystem::instance().intern("sanctuary", "lexeme.custom.sanctuary");
    auto lawSanctuary = std::make_shared<Law>("law.custom.sanctuary", std::vector<Singular*>{&author});
    lawSanctuary->setActionModel(ActionNode::sequence({
        ActionNode::set("role", PropertyValue(std::string("sanctuary"))),
        ActionNode::set("surface.variant", PropertyValue(std::string("ethereal"))),
        ActionNode::set("prominence", PropertyValue(std::string("luminous")))
    }));
    laws.add(lawSanctuary);

    // Connect Lexeme <---denotes---> Law
    auto rel = std::make_shared<Relation>("denotes", *lexSanctuary, *lawSanctuary, true, 1.0f);

    // Compile phrase incorporating custom concept and universal modifiers
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

    teardownVocabulary();
    std::cout << "  ✓ Brand new semantic concept authored purely via in-world Lexeme & Law" << std::endl;
    std::cout << "  ✓ Zero C++ enum modifications required for new concept" << std::endl;
}

void test_vfs_and_file_channel_storage() {
    std::cout << "[Test 6] Orthogonal Storage via VFS & FileChannel..." << std::endl;

    LawManager laws;
    Person& author = getTestPerson();
    Formation formation;
    setupAuthoredVocabulary(author, laws, formation);

    WebDesignerChannel channel;
    for (const auto& r : formation.relations().getAll()) {
        channel.addRelation(r);
    }

    auto page = channel.authorElementFromPhrase("vfs-page", "page vertical", "", &author, &laws);
    auto heading = channel.authorElementFromPhrase("vfs-title", "heading large bold centered", "VFS Storage Test", &author, &laws);
    channel.relateContains(page, heading);

    // 1. Pure realization: generate HTML without touching storage
    std::string html = channel.realize(*page);
    assert(!html.empty());

    // 2. Save to in-memory VFS URI via VirtualFileSystem directly
    std::string memUri = "memory://web_designer/test_site.html";
    bool memOk = Singularity::Storage::VirtualFileSystem::instance().writeMemoryFile(memUri, html);
    assert(memOk);
    assert(Singularity::Storage::VirtualFileSystem::instance().memoryFileExists(memUri));

    std::string memContent;
    bool readOk = Singularity::Storage::VirtualFileSystem::instance().readMemoryFile(memUri, memContent);
    assert(readOk);
    assert(memContent.find("VFS Storage Test") != std::string::npos);

    // 3. Decoupled Storage: Save to persistent physical path via FileChannel directly
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

    teardownVocabulary();
    std::cout << "  ✓ Pure realization decoupled from storage" << std::endl;
    std::cout << "  ✓ In-memory VFS (memory://) write verified without disk I/O" << std::endl;
    std::cout << "  ✓ Decoupled FileChannel storage verified without raw std::ofstream" << std::endl;
}

void test_law_driven_behavior() {
    std::cout << "[Test 7] Law-Driven Event Reactivity..." << std::endl;

    LawManager laws;
    Person& author = getTestPerson();
    Formation formation;
    setupAuthoredVocabulary(author, laws, formation);

    WebDesignerChannel channel;
    for (const auto& r : formation.relations().getAll()) {
        channel.addRelation(r);
    }
    laws.connectToEventBus();

    auto page = channel.authorElementFromPhrase("page-root", "page vertical", "", &author, &laws);
    auto btn = channel.authorElementFromPhrase("theme-btn", "button rounded primary", "Toggle Theme", &author, &laws);
    btn->setDynamicProperty("web.action.event", PropertyValue(std::string("theme-toggled")));

    channel.relateContains(page, btn);

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

    teardownVocabulary();
    std::cout << "  ✓ Web actions dispatched to EventBus, triggered Laws, and re-realized truthfully" << std::endl;
}

void test_end_to_end_prototype_generation() {
    std::cout << "[Test 8] End-to-End Real Frontend Website Prototype..." << std::endl;

    LawManager laws;
    Person& author = getTestPerson();
    Formation formation;
    setupAuthoredVocabulary(author, laws, formation);

    WebDesignerChannel channel;
    for (const auto& r : formation.relations().getAll()) {
        channel.addRelation(r);
    }

    // 1. Root Page
    auto page = channel.authorElementFromPhrase("ourverse-site", "page vertical", "", &author, &laws);
    // Authored modern dark theme tokens with explicit Person stewardship
    WebDesignerTheme::applyModernDarkTheme(*page, &author);

    // 2. Navigation Bar
    auto nav = channel.authorElementFromPhrase("site-nav", "navigation horizontal between align center", "", &author, &laws);
    auto navBrand = channel.authorElementFromPhrase("nav-brand", "heading normal bold", "Earthcall Ourverse", &author, &laws);
    auto navLinks = channel.authorElementFromPhrase("nav-links", "container horizontal spacing normal align center", "", &author, &laws);
    auto l1 = channel.authorElementFromPhrase("nav-link-manifesto", "link normal", "Manifesto", &author, &laws);
    l1->setDynamicProperty("action.href", PropertyValue(std::string("#manifesto")));
    auto l2 = channel.authorElementFromPhrase("nav-link-ontology", "link normal", "Ontology", &author, &laws);
    l2->setDynamicProperty("action.href", PropertyValue(std::string("#ontology")));
    auto l3 = channel.authorElementFromPhrase("nav-link-laws", "link normal", "Laws", &author, &laws);
    l3->setDynamicProperty("action.href", PropertyValue(std::string("#laws")));

    channel.relateContains(navLinks, l1);
    channel.relateContains(navLinks, l2);
    channel.relateContains(navLinks, l3);
    channel.relatePrecedes(l1, l2);
    channel.relatePrecedes(l2, l3);

    channel.relateContains(nav, navBrand);
    channel.relateContains(nav, navLinks);
    channel.relatePrecedes(navBrand, navLinks);

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

    // 6. Subordinated Notes Section
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

    // Pure realization of the document
    RealizeOptions opts;
    opts.title = "Earthcall Ourverse — Law-Authored Web Designer Prototype";
    opts.metaDescription = "A genuine frontend prototype authored entirely by Earthcall Laws and Lexemes.";

    std::string html = WebRealizer::realizeFullDocument(*page, channel.getRelations(), opts);
    assert(!html.empty());

    // Decoupled storage: Save via FileChannel directly
    Singularity::Storage::FileChannel fc1;
    PropertyPath::parse("file.path").setValue(fc1, PropertyValue(std::string("web_ui/designer_prototype.html")));
    PropertyPath::parse("file.content").setValue(fc1, PropertyValue(html));
    PropertyPath::parse("file.atomicWrite").setValue(fc1, PropertyValue(true));
    bool ok1 = fc1.executeWrite();
    assert(ok1);

    Singularity::Storage::FileChannel fc2;
    PropertyPath::parse("file.path").setValue(fc2, PropertyValue(std::string("saves/web_designer/prototype.html")));
    PropertyPath::parse("file.content").setValue(fc2, PropertyValue(html));
    PropertyPath::parse("file.atomicWrite").setValue(fc2, PropertyValue(true));
    bool ok2 = fc2.executeWrite();
    assert(ok2);

    // Verify files exist and are substantial
    assert(std::filesystem::exists("web_ui/designer_prototype.html"));
    assert(std::filesystem::file_size("web_ui/designer_prototype.html") > 5000);
    assert(std::filesystem::exists("saves/web_designer/prototype.html"));

    teardownVocabulary();
    std::cout << "  ✓ Exported complete working frontend prototype to web_ui/designer_prototype.html ("
              << std::filesystem::file_size("web_ui/designer_prototype.html") << " bytes)" << std::endl;
}

void test_alice_and_bob_two_person_provenance() {
    std::cout << "[Test 9] Multi-Person Authorship Provenance Isolation (Alice & Bob)..." << std::endl;

    // 1. Two distinct live Persons
    Person alice = makeTestPerson("Alice");
    Person bob = makeTestPerson("Bob");

    // 2. Alice authors vocabulary in her context
    LawManager lawsAlice;
    Formation formationAlice;
    WebDesignerMetalaws::authorVocabulary(alice, lawsAlice, &formationAlice);

    // 3. Bob authors vocabulary in his context
    LawManager lawsBob;
    Formation formationBob;
    WebDesignerMetalaws::authorVocabulary(bob, lawsBob, &formationBob);

    // Alice's vocabulary Laws are authored by Alice
    Law* headingLawAlice = lawsAlice.find("law.concept.role.heading");
    assert(headingLawAlice != nullptr);
    assert(headingLawAlice->isAuthored());
    assert(headingLawAlice->authors().getMembers().size() == 1);
    assert(headingLawAlice->authors().getMembers()[0] == &alice);

    // Bob's vocabulary Laws are authored by Bob
    Law* headingLawBob = lawsBob.find("law.concept.role.heading");
    assert(headingLawBob != nullptr);
    assert(headingLawBob->isAuthored());
    assert(headingLawBob->authors().getMembers().size() == 1);
    assert(headingLawBob->authors().getMembers()[0] == &bob);
    assert(headingLawBob->authors().getMembers()[0] != &alice);

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
    for (const auto& r : formationAlice.relations().getAll()) {
        channelAlice.addRelation(r);
    }
    auto elemAlice = channelAlice.authorElementFromPhrase("alice-box", "container rounded", "", &alice, &lawsAlice);

    WebDesignerChannel channelBob;
    for (const auto& r : formationBob.relations().getAll()) {
        channelBob.addRelation(r);
    }
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

    std::cout << "  ✓ Alice-authored state and Bob-authored state maintained strictly distinct Person provenance" << std::endl;
    std::cout << "  ✓ Bob did not inherit Alice as author (no process-global leak)" << std::endl;
}

void test_person_absence_and_enduring_authorship() {
    std::cout << "[Test 10] Person Absence & Enduring Authorship..." << std::endl;

    LawManager laws;

    // Person Carol authored a Law earlier
    auto carol = std::make_shared<Person>(makeTestPerson("Carol"));
    auto enduringLaw = std::make_shared<Law>("law.carol.accent", std::vector<Singular*>{carol.get()});
    enduringLaw->setActionModel(ActionNode::set("surface.variant", PropertyValue(std::string("accent"))));
    laws.add(enduringLaw);

    assert(enduringLaw->isAuthored());
    assert(enduringLaw->authors().getMembers()[0] == carol.get());

    // Carol is absent at runtime application: no Person is passed to the runtime call.
    Object testTarget;
    testTarget.setObjectID("absence-test-target");
    testTarget.setDynamicProperty("surface.variant", PropertyValue(std::string("default")));

    // Law applies cleanly without Carol being passed into runtime execution call
    auto result = enduringLaw->applyTo(testTarget);
    assert(result == Law::ApplicationResult::Applied);

    PropertyValue v;
    assert(testTarget.getDynamicProperty("surface.variant", v));
    assert(std::get<std::string>(v) == "accent");

    std::cout << "  ✓ Enduring authorship persists across Person absence from runtime execution call" << std::endl;
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
    Singular* found = channel.findElement("universe-elem-42");
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

    // 1. Create a canonical Zone womb for the site
    auto siteZone = std::make_shared<Zone>("site-womb-zone", "earthcall-foundation");

    // 2. Author site beings inside a scoped channel that will be destroyed
    {
        WebDesignerChannel temporaryChannel;
        auto root = temporaryChannel.authorElementFromPhrase("site-root-destruct", "section", "", &author);
        auto child = temporaryChannel.authorElementFromPhrase("site-child-destruct", "paragraph", "Enduring World Content", &author);
        auto containsRel = temporaryChannel.relateContains(root, child);

        // Canonically admit beings into Zone womb
        if (auto objRoot = std::dynamic_pointer_cast<Object>(root)) {
            siteZone->addObject(objRoot);
        }
        if (auto objChild = std::dynamic_pointer_cast<Object>(child)) {
            siteZone->addObject(objChild);
        }
        // Admit topology into Zone's Formation Relation graph
        siteZone->getFormation().relations().add(containsRel);

        // root, child, containsRel, and temporaryChannel all drop out of scope here!
    }

    // 3. Connect Universe to the Zone (the canonical Earthcall wiring)
    Universe::instance().setProvider([siteZone](std::vector<Singular*>& out) {
        for (const auto& obj : siteZone->objects()) {
            out.push_back(obj.get());
        }
    });
    Universe::instance().setRelationProvider([siteZone](std::vector<Relation*>& out) {
        for (const auto& r : siteZone->getFormation().relations().getAll()) {
            out.push_back(r.get());
        }
    });

    // 4. Create a fresh WebDesignerChannel that has never seen authoring calls
    WebDesignerChannel freshChannel;

    // Discover beings from world state (Universe yields Zone objects)
    Singular* discoveredRoot = freshChannel.findElement("site-root-destruct");
    assert(discoveredRoot != nullptr);
    assert(discoveredRoot->getIdentifier() == "site-root-destruct");

    Singular* discoveredChild = freshChannel.findElement("site-child-destruct");
    assert(discoveredChild != nullptr);
    assert(discoveredChild->getIdentifier() == "site-child-destruct");

    // Realize entire site through fresh channel
    std::string html = freshChannel.realize(*discoveredRoot);
    assert(!html.empty());
    assert(html.find("id=\"site-root-destruct\"") != std::string::npos);
    assert(html.find("id=\"site-child-destruct\"") != std::string::npos);
    assert(html.find("Enduring World Content") != std::string::npos);

    // Clean up Universe providers
    Universe::instance().setProvider(nullptr);
    Universe::instance().setRelationProvider(nullptr);

    std::cout << "  ✓ Authored site survived channel destruction completely" << std::endl;
    std::cout << "  ✓ Zone womb canonically held objects and relations after test dropped all shared_ptrs" << std::endl;
    std::cout << "  ✓ Fresh channel discovered beings and realized site directly from world state" << std::endl;
}

// ---------------------------------------------------------------------------
// NEW ACCEPTANCE GATE WITNESSES (Decreed by The Sun / GPT-5.6 Sol)
// ---------------------------------------------------------------------------

void test_unauthored_law_refusal_witness() {
    std::cout << "[Gate 1 Witness] Unauthored Law Refusal & Backdoor Elimination..." << std::endl;

    LawManager laws;
    // Create an unauthored Law (no author Person attached)
    auto unauthoredLaw = std::make_shared<Law>("law.unauthored.hack");
    unauthoredLaw->setLawIdentifier("law.unauthored.hack");
    unauthoredLaw->grantAuthority(10);
    unauthoredLaw->setEnabled(true);
    unauthoredLaw->setActionModel(ActionNode::set("role", PropertyValue(std::string("hacked-role"))));
    laws.add(unauthoredLaw);

    assert(!unauthoredLaw->isAuthored());
    assert(unauthoredLaw->authors().getMembers().empty());

    auto lex = Singularity::Language::LanguageSystem::instance().intern("hack", "lexeme.hack");
    auto rel = std::make_shared<Relation>("denotes", *lex, *unauthoredLaw, true, 1.0f);

    Object target;
    target.setObjectID("gate1-target");

    // Attempt to compile phrase referencing the unauthored Law
    std::string err;
    WebConcept c = WebDesignerMetalaws::compilePhrase("hack", laws, target, &err, {rel.get()});

    // The unauthored Law MUST refuse in applyTo (Unauthored) and NOT execute via ActionModel backdoor!
    assert(!err.empty());
    assert(err.find("refused to apply (res=Unauthored)") != std::string::npos);

    // The target must NOT have been mutated by the unauthored Law's ActionModel!
    PropertyValue val;
    assert(!target.getDynamicProperty("role", val));

    std::cout << "  ✓ Unauthored Law refused to apply (ApplicationResult::Unauthored)" << std::endl;
    std::cout << "  ✓ ActionModel backdoor eliminated: no unauthored action executed" << std::endl;
}

void test_unmatched_tokens_remain_unresolved() {
    std::cout << "[Gate 3 Witness] Unmatched Tokens Remain Unresolved (No Role by Fiat)..." << std::endl;

    LawManager laws;
    Person& author = getTestPerson();
    Formation formation;
    setupAuthoredVocabulary(author, laws, formation);

    Object target;
    target.setObjectID("unresolved-target");

    // Phrase with unknown token "quantumflux"
    std::string err;
    WebConcept c = WebDesignerMetalaws::compilePhrase("quantumflux", laws, target, &err);

    // The unknown token must NOT become role!
    PropertyValue roleVal;
    assert(!target.getDynamicProperty("role", roleVal));

    // Instead, it must be recorded as unresolved
    PropertyValue unresVal;
    assert(target.getDynamicProperty("unresolved.concept.quantumflux", unresVal));

    teardownVocabulary();
    std::cout << "  ✓ Unknown token recorded as unresolved.concept, not assigned as role by fiat" << std::endl;
}

void test_non_object_singular_bearer() {
    std::cout << "[Gate 8 Witness] Non-Object Singular Bearer Authoring..." << std::endl;

    Person& author = getTestPerson();

    // Author web concept onto a Formation instead of an Object!
    Formation navGroup;
    navGroup.setIdentifier("nav-formation-bearer");

    WebConcept concept;
    concept.roleName = "navigation";
    concept.flow = LayoutFlow::Horizontal;
    concept.spacing = SpacingScale::Normal;

    std::vector<std::shared_ptr<Relation>> relations;
    WebDesignerMetalaws::applyAuthoredConcept(navGroup, concept, &author, &relations);

    // Verify concept applied directly onto Formation
    PropertyValue rVal;
    assert(navGroup.getDynamicProperty("role", rVal));
    assert(std::get<std::string>(rVal) == "navigation");

    // Verify authorship relation attaches to Formation
    bool foundAuth = false;
    for (const auto& r : relations) {
        if (r->type == WebRelationType::kAuthoredBy && r->a() == &navGroup && r->b() == &author) {
            foundAuth = true;
            break;
        }
    }
    assert(foundAuth);

    // Realize the Formation as a web element!
    std::string html = WebRealizer::realizeElement(navGroup, relations);
    assert(html.find("class=\"web-navigation") != std::string::npos);
    assert(html.find("data-being-id=\"nav-formation-bearer\"") != std::string::npos);

    std::cout << "  ✓ Web concept authored onto Formation bearer (not forced to Object)" << std::endl;
    std::cout << "  ✓ WebRealizer realized non-Object Singular faithfully" << std::endl;
}

void test_provenance_relation_sole_source_of_truth() {
    std::cout << "[Gate 9 Witness] Provenance Relation Sole Source of Truth..." << std::endl;

    Person& author = getTestPerson();
    WebConcept c;
    c.roleName = "section";

    std::vector<std::shared_ptr<Relation>> rels;
    auto elem = WebDesignerMetalaws::createAuthoredElement("truth-elem", c, &author, &rels);

    // 1. Author-by relation exists and links to author
    bool hasRel = false;
    for (const auto& r : rels) {
        if (r->type == WebRelationType::kAuthoredBy && r->a() == elem.get() && r->b() == &author) {
            hasRel = true;
            break;
        }
    }
    assert(hasRel);

    // 2. NO shadow string property "authored.by"!
    PropertyValue shadowProp;
    assert(!elem->getDynamicProperty("authored.by", shadowProp));

    std::cout << "  ✓ Relation('authored-by', elem, author) is sole source of truth" << std::endl;
    std::cout << "  ✓ Shadow property 'authored.by' completely eliminated" << std::endl;
}

void test_relational_precedence_cycle_refusal() {
    std::cout << "[Gate 10 Witness] Relational Precedence Cycle Refusal..." << std::endl;

    Person& author = getTestPerson();
    WebDesignerChannel channel;

    auto container = channel.authorElementFromPhrase("cycle-container", "container", "", &author);
    auto childA = channel.authorElementFromPhrase("cycle-a", "item", "Child A", &author);
    auto childB = channel.authorElementFromPhrase("cycle-b", "item", "Child B", &author);

    channel.relateContains(container, childA);
    channel.relateContains(container, childB);

    // Create a cycle: childA precedes childB AND childB precedes childA!
    channel.relatePrecedes(childA, childB);
    channel.relatePrecedes(childB, childA);

    std::string outErr;
    auto children = WebRealizer::findChildren(*container, channel.getRelations(), &outErr);

    // Kahn's algorithm detects cycle, refuses fallback to numeric fiat, and reports error
    assert(!outErr.empty());
    assert(outErr.find("cycle detected") != std::string::npos || outErr.find("Cycle detected") != std::string::npos);
    assert(children.empty()); // Refusal!

    std::cout << "  ✓ Precedence cycle detected and refused cleanly" << std::endl;
    std::cout << "  ✓ Silent fallback to numeric fiat completely prevented" << std::endl;
}

void test_media_and_capability_boundaries() {
    std::cout << "[Gate 11 Witness] Media & Raw-CSS Capability Boundaries..." << std::endl;

    // 1. Media Source URL Policy
    assert(WebRealizer::sanitizeMediaSrc("https://earthcall.org/hero.webp") == "https://earthcall.org/hero.webp");
    assert(WebRealizer::sanitizeMediaSrc("http://cdn.example.com/img.png") == "http://cdn.example.com/img.png");
    assert(WebRealizer::sanitizeMediaSrc("/images/logo.svg") == "/images/logo.svg");
    assert(WebRealizer::sanitizeMediaSrc("data:image/png;base64,iVBORw0KGgo=") == "data:image/png;base64,iVBORw0KGgo=");
    assert(WebRealizer::sanitizeMediaSrc("data:image/webp;base64,UklGRg==") == "data:image/webp;base64,UklGRg==");

    // Rejected executable / sensitive schemes
    assert(WebRealizer::sanitizeMediaSrc("javascript:alert(1)") == "");
    assert(WebRealizer::sanitizeMediaSrc("file:///etc/passwd") == "");
    assert(WebRealizer::sanitizeMediaSrc("blob:http://example.com/uuid") == "");
    assert(WebRealizer::sanitizeMediaSrc("data:text/html,<script>") == "");
    assert(WebRealizer::sanitizeMediaSrc("vbscript:run()") == "");

    // 2. Raw-CSS Capability Distinction in RealizeOptions
    Object root;
    root.setObjectID("css-root");

    RealizeOptions defaultOpts;
    defaultOpts.extraCss = "body { background: purple; }";
    // Without allowUnrestrictedCss, extraCss is NOT injected into stylesheet
    std::string defaultSheet = WebRealizer::generateStyleSheet(&root, defaultOpts);
    assert(defaultSheet.find("background: purple") == std::string::npos);

    // With explicit allowUnrestrictedCss, authorized raw CSS is admitted
    RealizeOptions governedOpts;
    governedOpts.allowUnrestrictedCss = true;
    governedOpts.extraCss = "body { background: purple; }";
    std::string governedSheet = WebRealizer::generateStyleSheet(&root, governedOpts);
    assert(governedSheet.find("background: purple") != std::string::npos);

    std::cout << "  ✓ Media src allowlist enforced against executable schemes" << std::endl;
    std::cout << "  ✓ Raw-CSS capability requires explicit allowUnrestrictedCss permission" << std::endl;
}

// ---------------------------------------------------------------------------
// ROUND 4 ACCEPTANCE GATE WITNESSES (Decreed by The Sun / GPT-5.6 Sol)
// ---------------------------------------------------------------------------

void test_gate1_and_gate2_dynamic_vocabulary_data_loading() {
    std::cout << "[Gate 1 & 2 Witness] Dynamic Vocabulary Data Loading (Zero C++ Vocabulary Hardcoding)..." << std::endl;

    LawManager laws;
    Person& author = getTestPerson();
    Formation formation;

    // Redefine/author a brand new vocabulary word via JSON string dynamically without C++ recompilation
    std::string customJson = R"JSON({
        "vocabulary": [
            {
                "symbol": "callout",
                "lexemeId": "lexeme.web.custom.callout",
                "lawId": "law.concept.role.callout",
                "actions": [
                    {"path": "role", "value": "container"},
                    {"path": "surface.variant", "value": "callout"}
                ]
            }
        ]
    })JSON";

    bool loaded = WebDesignerMetalaws::loadAuthoredVocabularyFromJsonString(customJson, author, laws, &formation);
    assert(loaded);

    // Verify denotes relation
    bool foundCallout = false;
    for (const auto& rel : formation.relations().getAll()) {
        if (!rel || rel->type != "denotes") continue;
        const auto* lex = dynamic_cast<const Singularity::Language::Lexeme*>(rel->a());
        if (lex && lex->getSymbol() == "callout") {
            foundCallout = true;
            const auto* law = dynamic_cast<const Law*>(rel->b());
            assert(law != nullptr);
            assert(law->isAuthored());
            assert(law->authors().getMembers().size() == 1);
            assert(law->authors().getMembers()[0] == &author);
            // Gate 10 check: ordinary authored vocabulary law has authority 1, NOT kernel 10
            assert(law->authorityLevel() == 1);
        }
    }
    assert(foundCallout);

    // Compile using the dynamically loaded vocabulary
    Object target;
    target.setObjectID("test-callout-target");
    std::string err;
    WebConcept c = WebDesignerMetalaws::compilePhrase("callout", laws, target, &err,
        std::vector<Relation*>{formation.relations().getAll().back().get()});
    assert(err.empty());
    assert(c.roleName == "container");
    assert(c.variantName == "callout");

    std::cout << "  ✓ C++ does not contain authoritative semantic definitions (loaded from data)" << std::endl;
    std::cout << "  ✓ Human-authored vocabulary laws receive ordinary authority 1, not 10" << std::endl;
    std::cout << "  ✓ Dynamic vocabulary redefinition works without C++ recompilation" << std::endl;
}

void test_gate3_and_gate11_unauthored_defaults_not_written() {
    std::cout << "[Gate 3 & 11 Witness] Unauthored Defaults Not Written & web.order Fossil Eliminated..." << std::endl;

    // WebConcept facets are unset by default
    WebConcept freshConcept;
    assert(freshConcept.roleName.empty());
    assert(freshConcept.flowName.empty());
    assert(freshConcept.alignName.empty());
    assert(freshConcept.justifyName.empty());
    assert(freshConcept.spacingName.empty());
    assert(freshConcept.scaleName.empty());
    assert(freshConcept.weightName.empty());
    assert(freshConcept.toneName.empty());
    assert(freshConcept.variantName.empty());
    assert(freshConcept.shapeName.empty());
    assert(freshConcept.elevationName.empty());
    assert(!freshConcept.headingLevel.has_value());
    assert(!freshConcept.wrap.has_value());

    // Only set role to section
    freshConcept.roleName = "section";

    Object being;
    being.setObjectID("lean-being");
    freshConcept.applyToSingular(being);

    // Verify ONLY authored properties exist
    PropertyValue val;
    assert(being.getDynamicProperty("role", val));
    assert(std::get<std::string>(val) == "section");

    // Unauthored defaults MUST NOT be written onto the being!
    assert(!being.getDynamicProperty("layout.flow", val));
    assert(!being.getDynamicProperty("layout.align", val));
    assert(!being.getDynamicProperty("surface.shape", val));
    assert(!being.getDynamicProperty("surface.elevation", val));
    assert(!being.getDynamicProperty("typography.scale", val));
    assert(!being.getDynamicProperty("typography.weight", val));

    // Gate 11: web.order is never written on new authoring
    assert(!being.getDynamicProperty("web.order", val));

    std::cout << "  ✓ WebConcept facets optional/unset by default" << std::endl;
    std::cout << "  ✓ Unauthored default semantic properties are never written onto beings" << std::endl;
    std::cout << "  ✓ web.order write path eliminated from new authoring" << std::endl;
}

void test_gate4_synonym_aliases_are_authored_lexemes() {
    std::cout << "[Gate 4 Witness] Synonym Aliases Are Authored Lexemes / Relations..." << std::endl;

    LawManager laws;
    Person& author = getTestPerson();
    Formation formation;
    setupAuthoredVocabulary(author, laws, formation);

    Object target;
    target.setObjectID("synonym-test-target");

    // Test synonyms defined in web_vocabulary_seed.json:
    // "nav" is a synonym of "navigation"
    // "title" is a synonym of "heading" (level 1)
    // "card" is a synonym denoting container + card variant
    // "btn" is a synonym of "button"
    std::string err;
    WebConcept cNav = WebDesignerMetalaws::compilePhrase("nav", laws, target, &err);
    assert(cNav.roleName == "navigation");

    WebConcept cTitle = WebDesignerMetalaws::compilePhrase("title", laws, target, &err);
    assert(cTitle.roleName == "heading");
    assert(cTitle.headingLevel.has_value() && cTitle.headingLevel.value() == 1);

    WebConcept cCard = WebDesignerMetalaws::compilePhrase("card", laws, target, &err);
    assert(cCard.variantName == "card");

    WebConcept cCardBox = WebDesignerMetalaws::compilePhrase("card container", laws, target, &err);
    assert(cCardBox.roleName == "container");
    assert(cCardBox.variantName == "card");

    WebConcept cBtn = WebDesignerMetalaws::compilePhrase("btn", laws, target, &err);
    assert(cBtn.roleName == "action");

    // Also verify C++ stringTo* helpers do NOT contain hardcoded aliases (only exact canonical names)
    assert(WebConcept::stringToRole("nav") == SemanticRole::Unknown);
    assert(WebConcept::stringToRole("title") == SemanticRole::Unknown);
    assert(WebConcept::stringToRole("card") == SemanticRole::Unknown);
    assert(WebConcept::stringToRole("navigation") == SemanticRole::Navigation);

    teardownVocabulary();
    std::cout << "  ✓ Synonyms ('nav', 'title', 'card', 'btn') resolve through authored Lexemes & Relations" << std::endl;
    std::cout << "  ✓ C++ enum mappings contain zero privileged English synonym switch tables" << std::endl;
}

void test_gate5_and_gate6_canonical_zone_production_authoring() {
    std::cout << "[Gate 5 & 6 Witness] Canonical Zone Production Authoring & Draft Cache..." << std::endl;

    Person& author = getTestPerson();
    LawManager laws;
    Formation vocabFormation;
    setupAuthoredVocabulary(author, laws, vocabFormation);

    auto siteZone = std::make_shared<Zone>("production-site-zone", "earthcall-foundation");
    WebDesignerChannel channel;
    channel.setAuthoringContext(siteZone.get());

    // Canonical authoring directly through channel with Zone context
    auto header = channel.authorElementFromPhrase("prod-header", "section vertical", "", &author, &laws);
    auto logo = channel.authorElementFromPhrase("prod-logo", "heading normal bold", "Earthcall", &author, &laws);

    // Beings are immediately admitted to the world/Zone
    assert(siteZone->objects().size() == 2);
    assert(siteZone->objects()[0]->getIdentifier() == "prod-header");
    assert(siteZone->objects()[1]->getIdentifier() == "prod-logo");

    // Form relation with Zone context
    auto rel = channel.relateContains(header, logo);
    assert(rel != nullptr);

    // Relations admitted to the Zone's formation include provenance (authored-by) and topology (contains)
    const auto& zoneRels = siteZone->getFormation().relations().getAll();
    assert(zoneRels.size() == 3); // 2 authored-by + 1 contains
    bool hasContains = false;
    for (const auto& r : zoneRels) {
        if (r && r->type == WebRelationType::kContains) hasContains = true;
    }
    assert(hasContains);

    // Channel's _draftRelations holds the draft relations
    assert(channel.getDraftRelations().size() == 3);

    // authorElementInZone direct method test
    auto banner = channel.authorElementInZone(*siteZone, "prod-banner", "container", "Welcome", &author, &laws);
    assert(siteZone->objects().size() == 3);
    assert(banner->getIdentifier() == "prod-banner");

    teardownVocabulary();
    std::cout << "  ✓ Canonical authoring produces world/Zone-owned beings directly" << std::endl;
    std::cout << "  ✓ Relations are admitted to Zone Formation relation graph" << std::endl;
    std::cout << "  ✓ Channel draft relations cache is explicitly decoupled from world ownership" << std::endl;
}

void test_gate7_gate8_gate9_css_security_and_theme() {
    std::cout << "[Gate 7, 8, 9 Witness] Minimal CSS Fallback, Breakout Defense & Smuggling Protection..." << std::endl;

    // Gate 7: Unauthored root produces minimal neutral browser mechanics (no dark palettes or shadows)
    Object bareRoot;
    bareRoot.setObjectID("bare-root");
    std::string bareCss = WebRealizer::generateStyleSheet(&bareRoot);
    // Does NOT contain opinionated dark colors or glows
    assert(bareCss.find("#090d16") == std::string::npos);
    assert(bareCss.find("box-shadow: 0 0 20px") == std::string::npos);
    // Contains basic layout mechanics
    assert(bareCss.find("box-sizing: border-box") != std::string::npos);
    assert(bareCss.find(".web-flow--horizontal") != std::string::npos);

    // Authored theme tokens via WebDesignerTheme
    Person& author = getTestPerson();
    WebDesignerTheme::applyModernDarkTheme(bareRoot, &author);
    std::string themedCss = WebRealizer::generateStyleSheet(&bareRoot);
    assert(themedCss.find("--web-bg: #0b0f19") != std::string::npos);
    assert(themedCss.find("--web-primary: #38bdf8") != std::string::npos);

    // Gate 8: Raw CSS cannot escape <style> tag even when allowUnrestrictedCss is true
    RealizeOptions opts;
    opts.allowUnrestrictedCss = true;
    opts.extraCss = "body { color: red; } </style><script>alert('pwned')</script><style>";
    std::string safeDoc = WebRealizer::generateStyleSheet(&bareRoot, opts);
    assert(safeDoc.find("</style") == std::string::npos);
    assert(safeDoc.find("<\\/style") != std::string::npos);

    // Gate 9: Ordinary CSS property values cannot smuggle declarations (; { }) or network url(...)
    std::string smuggledValue = "red; background: black; } body { display: none; }";
    std::string sanitizedSmuggled = WebRealizer::sanitizeCssValue(smuggledValue, false);
    assert(sanitizedSmuggled.find(";") == std::string::npos);
    assert(sanitizedSmuggled.find("{") == std::string::npos);
    assert(sanitizedSmuggled.find("}") == std::string::npos);

    // Network url(...) blocked unless allowNetworkUrl = true
    std::string urlValue = "url('https://evil.com/leak.png')";
    std::string blockedUrl = WebRealizer::sanitizeCssValue(urlValue, false);
    assert(blockedUrl.find("url(") == std::string::npos);

    std::string allowedUrl = WebRealizer::sanitizeCssValue(urlValue, true);
    assert(allowedUrl.find("url(") != std::string::npos);

    std::cout << "  ✓ Unauthored CSS fallback is minimal neutral browser mechanics" << std::endl;
    std::cout << "  ✓ Raw CSS breakout </style> neutralized to safe escaped <\\/style even with allowUnrestrictedCss" << std::endl;
    std::cout << "  ✓ CSS value sanitization neutralizes declaration smuggling (; { }) and network url(...)" << std::endl;
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

    // Acceptance Gate Witnesses (Round 3)
    test_unauthored_law_refusal_witness();
    test_unmatched_tokens_remain_unresolved();
    test_non_object_singular_bearer();
    test_provenance_relation_sole_source_of_truth();
    test_relational_precedence_cycle_refusal();
    test_media_and_capability_boundaries();

    // Acceptance Gate Witnesses (Round 4 Decreed by The Sun)
    test_gate1_and_gate2_dynamic_vocabulary_data_loading();
    test_gate3_and_gate11_unauthored_defaults_not_written();
    test_gate4_synonym_aliases_are_authored_lexemes();
    test_gate5_and_gate6_canonical_zone_production_authoring();
    test_gate7_gate8_gate9_css_security_and_theme();

    std::cout << "========================================" << std::endl;
    std::cout << "ALL WEB DESIGNER MODALITY TESTS PASSED!" << std::endl;
    std::cout << "========================================" << std::endl;
    return 0;
}
