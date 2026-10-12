#include "Singularity/Foreign/Web/Designer/WebConcept.hpp"
#include "Singularity/Foreign/Web/Designer/WebDesignerMetalaws.hpp"
#include "Singularity/Foreign/Web/Designer/WebRealizer.hpp"
#include "Singularity/Foreign/Web/Designer/WebDesignerChannel.hpp"
#include "ZonesOfEarth/AuthorsOfLaw/Law.hpp"
#include "ConstructedBeing/Singular/Object/Object.hpp"
#include "Singularity/Core/EventBus.hpp"

#include <cassert>
#include <iostream>
#include <filesystem>
#include <fstream>

using namespace Singularity::Foreign::Web;

void test_lexeme_resolution() {
    std::cout << "[Test 1] Lexeme Resolution..." << std::endl;

    // Verify lookup of representative Lexemes
    const auto* h = WebDesignerMetalaws::findDefinition("heading");
    assert(h != nullptr);
    assert(h->opcode == "web.concept.role.heading");
    assert(h->stableId == "lexeme.web.heading");

    const auto* p = WebDesignerMetalaws::findDefinition("paragraph");
    assert(p != nullptr);
    assert(p->opcode == "web.concept.role.paragraph");

    const auto* btn = WebDesignerMetalaws::findDefinition("button");
    assert(btn != nullptr);
    assert(btn->opcode == "web.concept.role.action");

    const auto* nav = WebDesignerMetalaws::findDefinition("navigation");
    assert(nav != nullptr);
    assert(nav->opcode == "web.concept.role.navigation");

    const auto* sec = WebDesignerMetalaws::findDefinition("section");
    assert(sec != nullptr);
    assert(sec->opcode == "web.concept.role.section");

    const auto* card = WebDesignerMetalaws::findDefinition("card");
    assert(card != nullptr);
    assert(card->opcode == "web.concept.role.card");

    const auto* v = WebDesignerMetalaws::findDefinition("vertical");
    assert(v != nullptr);
    assert(v->opcode == "web.concept.flow.vertical");

    const auto* horiz = WebDesignerMetalaws::findDefinition("horizontal");
    assert(horiz != nullptr);
    assert(horiz->opcode == "web.concept.flow.horizontal");

    const auto* center = WebDesignerMetalaws::findDefinition("centered");
    assert(center != nullptr);
    assert(center->opcode == "web.concept.align.center");

    const auto* lg = WebDesignerMetalaws::findDefinition("large");
    assert(lg != nullptr);
    assert(lg->opcode == "web.concept.size.large");

    const auto* bld = WebDesignerMetalaws::findDefinition("bold");
    assert(bld != nullptr);
    assert(bld->opcode == "web.concept.weight.bold");

    const auto* rnd = WebDesignerMetalaws::findDefinition("rounded");
    assert(rnd != nullptr);
    assert(rnd->opcode == "web.concept.shape.rounded");

    const auto* gen = WebDesignerMetalaws::findDefinition("generous");
    assert(gen != nullptr);
    assert(gen->opcode == "web.concept.spacing.generous");

    const auto* pri = WebDesignerMetalaws::findDefinition("primary");
    assert(pri != nullptr);
    assert(pri->opcode == "web.concept.variant.primary");

    std::cout << "  ✓ All representative Lexemes resolved correctly" << std::endl;
}

void test_metalaw_composition() {
    std::cout << "[Test 2] Metalaw Composition..." << std::endl;

    LawManager laws;
    Object author;
    author.setObjectID("Zach");
    Object channelTarget;
    channelTarget.setObjectID("test-channel-target");

    WebDesignerMetalaws::syncRegister(laws, &author);

    // 1. "a large centered heading"
    {
        std::string err;
        WebConcept c = WebDesignerMetalaws::compilePhrase(
            "a large centered heading", laws, channelTarget, &err);
        assert(err.empty());
        assert(c.role == SemanticRole::Heading);
        assert(c.headingLevel == 2);
        assert(c.scale == ContentScale::Large);
        assert(c.align == LayoutAlign::Center);
    }

    // 2. "a rounded primary button"
    {
        std::string err;
        WebConcept c = WebDesignerMetalaws::compilePhrase(
            "a rounded primary button", laws, channelTarget, &err);
        assert(err.empty());
        assert(c.role == SemanticRole::Action);
        assert(c.variant == SurfaceVariant::Primary);
        assert(c.shape == SurfaceShape::Rounded);
    }

    // 3. "a vertical section with generous spacing"
    {
        std::string err;
        WebConcept c = WebDesignerMetalaws::compilePhrase(
            "a vertical section with generous spacing", laws, channelTarget, &err);
        assert(err.empty());
        assert(c.role == SemanticRole::Section);
        assert(c.flow == LayoutFlow::Vertical);
        assert(c.spacing == SpacingScale::Generous);
    }

    // 4. "card container subtle elevated"
    {
        std::string err;
        WebConcept c = WebDesignerMetalaws::compilePhrase(
            "card container subtle", laws, channelTarget, &err);
        assert(err.empty());
        assert(c.role == SemanticRole::Container);
        assert(c.variant == SurfaceVariant::Card);
        assert(c.elevation == SurfaceElevation::Subtle);
    }

    // 5. Semantic conflict detection: "horizontal vertical"
    {
        std::string err;
        WebConcept c = WebDesignerMetalaws::compilePhrase(
            "horizontal vertical", laws, channelTarget, &err);
        assert(!err.empty());
        assert(err.find("Conflicting layout flow") != std::string::npos);
    }

    std::cout << "  ✓ Metalaws composed orthogonal concepts & detected conflicts correctly" << std::endl;
}

void test_law_authored_hierarchy() {
    std::cout << "[Test 3] Law-Authored Hierarchy & Relations..." << std::endl;

    LawManager laws;
    Object author;
    author.setObjectID("Zach");
    WebDesignerChannel channel;
    WebDesignerMetalaws::syncRegister(laws, &author);

    // Create page
    auto page = channel.authorElementFromPhrase("site-page", "page vertical", "", &author, &laws);
    assert(page != nullptr);

    // Create section
    auto section = channel.authorElementFromPhrase("hero-section", "section vertical centered generous", "", &author, &laws);
    assert(section != nullptr);

    // Create heading
    auto heading = channel.authorElementFromPhrase("hero-heading", "heading large bold centered", "Earthcall Sanctuary", &author, &laws);
    assert(heading != nullptr);

    // Create button
    auto btn = channel.authorElementFromPhrase("hero-btn", "button rounded primary", "Enter", &author, &laws);
    assert(btn != nullptr);

    // Form containment relations
    auto r1 = channel.relateContains(page, section, 1);
    auto r2 = channel.relateContains(section, heading, 1);
    auto r3 = channel.relateContains(section, btn, 2);

    assert(r1 != nullptr);
    assert(r2 != nullptr);
    assert(r3 != nullptr);
    assert(channel.getRelations().size() == 3);

    // Verify properties on Singulars
    WebConcept pageConcept = WebConcept::readFromSingular(*page);
    assert(pageConcept.role == SemanticRole::Page);

    WebConcept headingConcept = WebConcept::readFromSingular(*heading);
    assert(headingConcept.role == SemanticRole::Heading);
    assert(headingConcept.text == "Earthcall Sanctuary");
    assert(headingConcept.scale == ContentScale::Large);

    std::cout << "  ✓ Hierarchy and relations established with intact in-world properties" << std::endl;
}

void test_html_dom_realization() {
    std::cout << "[Test 4] HTML / CSS / DOM Realization..." << std::endl;

    LawManager laws;
    Object author;
    author.setObjectID("Zach");
    WebDesignerChannel channel;
    WebDesignerMetalaws::syncRegister(laws, &author);

    auto page = channel.authorElementFromPhrase("page-root", "page vertical", "", &author, &laws);
    auto nav = channel.authorElementFromPhrase("main-nav", "navigation horizontal between", "", &author, &laws);
    auto brand = channel.authorElementFromPhrase("nav-brand", "heading normal bold", "Earthcall", &author, &laws);
    auto navBtn = channel.authorElementFromPhrase("nav-action", "button rounded primary", "Sign In", &author, &laws);

    channel.relateContains(page, nav, 1);
    channel.relateContains(nav, brand, 1);
    channel.relateContains(nav, navBtn, 2);

    RealizeOptions opts;
    opts.title = "Test Realization";
    std::string html = channel.realize(*page, opts);

    assert(html.find("<!DOCTYPE html>") != std::string::npos);
    assert(html.find("<nav id=\"main-nav\"") != std::string::npos);
    assert(html.find("role=\"navigation\"") != std::string::npos);
    assert(html.find("Earthcall") != std::string::npos);
    assert(html.find("Sign In") != std::string::npos);
    assert(html.find("class=\"web-heading") != std::string::npos);
    assert(html.find("class=\"web-action") != std::string::npos);
    assert(html.find("web-shape--rounded") != std::string::npos);
    assert(html.find("web-surface--primary") != std::string::npos);
    assert(html.find("<style>") != std::string::npos);
    assert(html.find("--web-primary:") != std::string::npos);

    std::cout << "  ✓ HTML5 markup, semantic landmarks, and CSS classes realized truthfully" << std::endl;
}

void test_law_driven_behavior() {
    std::cout << "[Test 5] Law-Driven Dynamic Behavior..." << std::endl;

    LawManager laws;
    Object author;
    author.setObjectID("Zach");
    WebDesignerChannel channel;
    WebDesignerMetalaws::syncRegister(laws, &author);
    laws.connectToEventBus();

    auto page = channel.authorElementFromPhrase("page-root", "page vertical", "", &author, &laws);
    auto btn = channel.authorElementFromPhrase("theme-btn", "button rounded primary", "Toggle Theme", &author, &laws);
    btn->setDynamicProperty("web.action.event", PropertyValue(std::string("theme-toggled")));

    channel.relateContains(page, btn, 1);

    // Initial realization: button element markup should be primary, not secondary
    std::string btnMarkupInitial = WebRealizer::realizeElement(*btn, channel.getRelations(), 0);
    assert(btnMarkupInitial.find("web-surface--primary") != std::string::npos);
    assert(btnMarkupInitial.find("web-surface--secondary") == std::string::npos);

    // Author a reactive Law that changes the button's style when "theme-toggled" is heard
    auto reactiveLaw = std::make_shared<Law>("theme-switcher-law", std::vector<Singular*>{&author});
    reactiveLaw->setLawIdentifier("theme-switcher-law");
    reactiveLaw->grantAuthority(5);
    reactiveLaw->setActivation(Law::Activation::OnEvent);
    reactiveLaw->setScope(Law::Scope::Subject);
    reactiveLaw->addTarget(*btn);
    reactiveLaw->setActionModel(ActionNode::set("web.surface.variant", PropertyValue(std::string("secondary"))));

    laws.add(reactiveLaw);
    laws.bindTrigger("theme-switcher-law", "theme-toggled");

    // Sensed web action: user clicks button in webpage
    channel.handleWebAction("theme-btn", "theme-toggled", laws);

    // Verify the Law actually executed and changed the being's property!
    PropertyValue variantVal;
    assert(btn->getDynamicProperty("web.surface.variant", variantVal));
    assert(std::get<std::string>(variantVal) == "secondary");

    // Re-realize: DOM now reflects the Law's mutation!
    std::string btnMarkupAfter = WebRealizer::realizeElement(*btn, channel.getRelations(), 0);
    assert(btnMarkupAfter.find("web-surface--secondary") != std::string::npos);
    assert(btnMarkupAfter.find("web-surface--primary") == std::string::npos);

    std::cout << "  ✓ User interaction triggers Earthcall Law, mutating in-world web state" << std::endl;
}

void test_end_to_end_prototype_generation() {
    std::cout << "[Test 6] End-to-End Nontrivial Website Prototype..." << std::endl;

    LawManager laws;
    Object author;
    author.setObjectID("Zach");
    WebDesignerChannel channel;
    WebDesignerMetalaws::syncRegister(laws, &author);

    // Author complete website structure through Laws & Lexemes
    // 1. Root Page
    auto page = channel.authorElementFromPhrase("earthcall-site", "page vertical spacing generous", "", &author, &laws);

    // 2. Navigation Bar
    auto nav = channel.authorElementFromPhrase("main-navigation", "navigation horizontal between align center", "", &author, &laws);
    auto brand = channel.authorElementFromPhrase("nav-brand", "heading normal bold", "Earthcall Ourverse", &author, &laws);
    brand->setDynamicProperty("web.action.href", PropertyValue(std::string("#")));
    auto navLinks = channel.authorElementFromPhrase("nav-links", "container horizontal align center spacing normal", "", &author, &laws);
    auto l1 = channel.authorElementFromPhrase("nav-link-manifesto", "link normal", "Manifesto", &author, &laws);
    l1->setDynamicProperty("web.action.href", PropertyValue(std::string("#manifesto")));
    auto l2 = channel.authorElementFromPhrase("nav-link-ontology", "link normal", "Ontology", &author, &laws);
    l2->setDynamicProperty("web.action.href", PropertyValue(std::string("#ontology")));
    auto l3 = channel.authorElementFromPhrase("nav-link-laws", "link normal", "Living Laws", &author, &laws);
    l3->setDynamicProperty("web.action.href", PropertyValue(std::string("#laws")));
    auto navCta = channel.authorElementFromPhrase("nav-cta", "button rounded primary", "Enter Ourverse", &author, &laws);
    navCta->setDynamicProperty("web.action.event", PropertyValue(std::string("enter-ourverse")));

    channel.relateContains(navLinks, l1, 1);
    channel.relateContains(navLinks, l2, 2);
    channel.relateContains(navLinks, l3, 3);
    channel.relateContains(nav, brand, 1);
    channel.relateContains(nav, navLinks, 2);
    channel.relateContains(nav, navCta, 3);
    channel.relateContains(page, nav, 1);

    // 3. Hero Section
    auto hero = channel.authorElementFromPhrase("hero-section", "section vertical centered generous hero", "", &author, &laws);
    auto heroTitle = channel.authorElementFromPhrase("hero-title", "heading huge bold centered", "Earthcall Ourverse", &author, &laws);
    auto heroLead = channel.authorElementFromPhrase("hero-lead", "paragraph large lead centered",
        "A Person-centered ontology that orders the engine attached to it. A vessel of shared human life that glorifies God and holds encounters with Christ and each other.", &author, &laws);
    auto heroActions = channel.authorElementFromPhrase("hero-actions", "container horizontal centered spacing normal", "", &author, &laws);
    auto ctaPrimary = channel.authorElementFromPhrase("hero-cta-primary", "button rounded primary large", "Explore Living Laws", &author, &laws);
    ctaPrimary->setDynamicProperty("web.action.event", PropertyValue(std::string("explore-laws-clicked")));
    auto ctaSecondary = channel.authorElementFromPhrase("hero-cta-secondary", "button rounded outline large", "Read The Manifesto", &author, &laws);
    ctaSecondary->setDynamicProperty("web.action.href", PropertyValue(std::string("#manifesto")));

    channel.relateContains(heroActions, ctaPrimary, 1);
    channel.relateContains(heroActions, ctaSecondary, 2);
    channel.relateContains(hero, heroTitle, 1);
    channel.relateContains(hero, heroLead, 2);
    channel.relateContains(hero, heroActions, 3);
    channel.relateContains(page, hero, 2);

    // 4. Core Pillars / Features Section
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
    channel.relateContains(c1, c1Title, 1);
    channel.relateContains(c1, c1Text, 2);
    channel.relateContains(c1, c1Btn, 3);

    // Card 2
    auto c2 = channel.authorElementFromPhrase("card-laws", "card container rounded subtle", "", &author, &laws);
    auto c2Title = channel.authorElementFromPhrase("card-laws-title", "heading normal bold", "Living Laws & Metalaws", &author, &laws);
    auto c2Text = channel.authorElementFromPhrase("card-laws-text", "paragraph normal muted",
        "Behavior emerges from Person-authored Laws, not hardcoded algorithms. Metalaws govern and compile authored sentences without a black box.", &author, &laws);
    auto c2Btn = channel.authorElementFromPhrase("card-laws-btn", "link normal", "View law engine →", &author, &laws);
    channel.relateContains(c2, c2Title, 1);
    channel.relateContains(c2, c2Text, 2);
    channel.relateContains(c2, c2Btn, 3);

    // Card 3
    auto c3 = channel.authorElementFromPhrase("card-ontomath", "card container rounded subtle", "", &author, &laws);
    auto c3Title = channel.authorElementFromPhrase("card-ontomath-title", "heading normal bold", "OntoMath Continuous Fields", &author, &laws);
    auto c3Text = channel.authorElementFromPhrase("card-ontomath-text", "paragraph normal muted",
        "Mathematical meaning owned in closed form: continuous procedural manifolds, signed distance fields, and exact analytic rendering.", &author, &laws);
    auto c3Btn = channel.authorElementFromPhrase("card-ontomath-btn", "link normal", "Explore geometry →", &author, &laws);
    channel.relateContains(c3, c3Title, 1);
    channel.relateContains(c3, c3Text, 2);
    channel.relateContains(c3, c3Btn, 3);

    channel.relateContains(featGrid, c1, 1);
    channel.relateContains(featGrid, c2, 2);
    channel.relateContains(featGrid, c3, 3);
    channel.relateContains(featuresSec, featTitle, 1);
    channel.relateContains(featuresSec, featSub, 2);
    channel.relateContains(featuresSec, featGrid, 3);
    channel.relateContains(page, featuresSec, 3);

    // 5. Interactive Law Demonstration Section
    auto demoSec = channel.authorElementFromPhrase("demo-section", "section vertical centered generous", "", &author, &laws);
    demoSec->setDynamicProperty("id", PropertyValue(std::string("laws")));
    auto demoTitle = channel.authorElementFromPhrase("demo-title", "heading large bold centered", "Live Law Interactivity Witness", &author, &laws);
    auto demoCard = channel.authorElementFromPhrase("demo-card", "card container rounded subtle", "", &author, &laws);
    auto demoText = channel.authorElementFromPhrase("demo-text", "paragraph normal centered",
        "Every element on this page was authored by Earthcall Laws and Lexemes. Click below to verify live EventBus event firing:", &author, &laws);
    auto demoBtn = channel.authorElementFromPhrase("demo-btn", "button rounded primary", "Invoke Law Event (Test)", &author, &laws);
    demoBtn->setDynamicProperty("web.action.event", PropertyValue(std::string("interactive-witness-clicked")));
    channel.relateContains(demoCard, demoText, 1);
    channel.relateContains(demoCard, demoBtn, 2);
    channel.relateContains(demoSec, demoTitle, 1);
    channel.relateContains(demoSec, demoCard, 2);
    channel.relateContains(page, demoSec, 4);

    // 6. Manifesto Quote Section
    auto quoteSec = channel.authorElementFromPhrase("quote-section", "section vertical centered relaxed", "", &author, &laws);
    quoteSec->setDynamicProperty("id", PropertyValue(std::string("manifesto")));
    auto quote = channel.authorElementFromPhrase("manifesto-quote", "quote centered prominent",
        "\"Earthcall is a Person-centered ontology that orders the engine attached to it. The engine here is not the order of truth in Earthcall—the ontology is, and the engine serves as the vessel for that.\" — Zachary Zhang", &author, &laws);
    channel.relateContains(quoteSec, quote, 1);
    channel.relateContains(page, quoteSec, 5);

    // 7. Footer
    auto footer = channel.authorElementFromPhrase("footer-section", "section horizontal between align center", "", &author, &laws);
    auto footerText = channel.authorElementFromPhrase("footer-text", "paragraph small muted",
        "© 2026 Earthcall Ourverse. Authored natively by Earthcall Laws, Lexemes, and Metalaws.", &author, &laws);
    auto footerLinks = channel.authorElementFromPhrase("footer-links", "container horizontal spacing normal", "", &author, &laws);
    auto fl1 = channel.authorElementFromPhrase("footer-link-1", "link small muted", "Seven Refusals", &author, &laws);
    auto fl2 = channel.authorElementFromPhrase("footer-link-2", "link small muted", "Agent Compass", &author, &laws);
    channel.relateContains(footerLinks, fl1, 1);
    channel.relateContains(footerLinks, fl2, 2);
    channel.relateContains(footer, footerText, 1);
    channel.relateContains(footer, footerLinks, 2);
    channel.relateContains(page, footer, 6);

    // Realize and export to web_ui/designer_prototype.html and saves/web_designer/prototype.html
    RealizeOptions opts;
    opts.title = "Earthcall Ourverse — Law-Authored Web Designer Prototype";
    opts.metaDescription = "A genuine frontend prototype authored entirely by Earthcall Laws and Lexemes.";

    bool ok1 = channel.exportHtml(*page, "web_ui/designer_prototype.html", opts);
    assert(ok1);
    bool ok2 = channel.exportHtml(*page, "saves/web_designer/prototype.html", opts);
    assert(ok2);

    // Verify files exist and are substantial
    assert(std::filesystem::exists("web_ui/designer_prototype.html"));
    assert(std::filesystem::file_size("web_ui/designer_prototype.html") > 5000);
    assert(std::filesystem::exists("saves/web_designer/prototype.html"));

    std::cout << "  ✓ Exported complete working frontend prototype to web_ui/designer_prototype.html ("
              << std::filesystem::file_size("web_ui/designer_prototype.html") << " bytes)" << std::endl;
}

int main() {
    std::cout << "========================================" << std::endl;
    std::cout << "Running Web Designer Modality Test Suite" << std::endl;
    std::cout << "========================================" << std::endl;

    test_lexeme_resolution();
    test_metalaw_composition();
    test_law_authored_hierarchy();
    test_html_dom_realization();
    test_law_driven_behavior();
    test_end_to_end_prototype_generation();

    std::cout << "========================================" << std::endl;
    std::cout << "ALL WEB DESIGNER MODALITY TESTS PASSED!" << std::endl;
    std::cout << "========================================" << std::endl;
    return 0;
}
