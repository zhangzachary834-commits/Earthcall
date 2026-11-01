#include "Person/Body/BodyPart/BodyPart.hpp"
#include "Singularity/Screen/Renderer.hpp"
#include "ConstructedBeing/Singular/Property/PropertyValue.hpp"
#include <cassert>
#include <cmath>
#include <iostream>
#include <glm/gtc/matrix_transform.hpp>

// MockRenderer to test draw() without needing an active OpenGL or GPU window context
class MockRenderer : public Renderer {
public:
    int drawMeshCount = 0;

    void drawMesh(const geom::TessMesh&, const RenderMaterial&) override {
        drawMeshCount++;
    }
    void drawImplicit(const geom::SdfNode&, const glm::vec3&, const RenderMaterial&,
                      const geom::FieldNode*, uint64_t, uint32_t, const geom::HeightGrid*, uint32_t) override {}
    void drawLines(const std::vector<std::pair<glm::vec3, glm::vec3>>&, const glm::vec4&, float, Blend) override {}
    void drawOverlay(const geom::TessMesh&, const glm::vec4&, float, bool) override {}
    void drawSolid(const std::vector<glm::vec3>&, const glm::vec4&, Blend, bool) override {}
    void begin2D(uint32_t, uint32_t) override {}
    void end2D() override {}
    void drawTris2D(const std::vector<glm::vec2>&, const glm::vec4&) override {}
    void drawLines2D(const std::vector<glm::vec2>&, const glm::vec4&, float) override {}
    void drawImage2D(const uint8_t*, uint32_t, uint32_t, const glm::vec4&, const glm::vec4&) override {}
    TextureHandle uploadTexture(TextureHandle, const uint8_t*, uint32_t, uint32_t) override { return 1; }
    void releaseTexture(TextureHandle) override {}
};

static void testInitialState() {
    BodyPart bodyPart;
    assert(bodyPart.getName() == "");
    assert(bodyPart.getType() == BodyPart::Type::Undefined);
    assert(bodyPart.getPrimaryObject()->getShapeKind() == ObjectTypes::ShapeKind::Cube);

    // Check default color
    const float* color = bodyPart.getColor();
    assert(color[0] == 1.0f);
    assert(color[1] == 1.0f);
    assert(color[2] == 1.0f);

    // Check default boolean flags
    assert(bodyPart.isLiteral == true);
    assert(bodyPart.isSymbolic == false);

    std::cout << "  testInitialState OK\n";
}

static void testCustomInitialization() {
    BodyPart bodyPart("RightArm", BodyPart::Type::Arm, ObjectTypes::ShapeKind::Sphere, glm::vec3(2.0f, 3.0f, 4.0f));
    assert(bodyPart.getName() == "RightArm");
    assert(bodyPart.getType() == BodyPart::Type::Arm);
    assert(bodyPart.getPrimaryObject()->getShapeKind() == ObjectTypes::ShapeKind::Sphere);

    glm::vec3 dims = bodyPart.getDimensions();
    assert(dims.x == 2.0f);
    assert(dims.y == 3.0f);
    assert(dims.z == 4.0f);

    // Check primary object name
    assert(bodyPart.getPrimaryObject() != nullptr);
    assert(bodyPart.getPrimaryObject()->getIdentifier() == "bodypart.RightArm");

    // Test constructor with initial transform
    glm::mat4 initialT = glm::translate(glm::mat4(1.0f), glm::vec3(1.0f, 2.0f, 3.0f));
    BodyPart bodyPartWithTransform("LeftLeg", BodyPart::Type::Leg, ObjectTypes::ShapeKind::Cylinder,
                                   glm::vec3(1.0f, 2.0f, 1.0f), initialT);
    assert(bodyPartWithTransform.getName() == "LeftLeg");
    assert(bodyPartWithTransform.getType() == BodyPart::Type::Leg);
    assert(bodyPartWithTransform.localTransform() == initialT);
    assert(bodyPartWithTransform.getTransform() == initialT);
    assert(bodyPartWithTransform.getPrimaryObject()->getTransform() == initialT);

    std::cout << "  testCustomInitialization OK\n";
}

static void testAccessorsAndMutators() {
    BodyPart bodyPart("Head", BodyPart::Type::Head, ObjectTypes::ShapeKind::Sphere, glm::vec3(1.0f));

    // Identifier check
    assert(bodyPart.getIdentifier() == "Head");

    // Color mutator/accessor
    bodyPart.setColor(0.5f, 0.6f, 0.7f);
    const float* col = bodyPart.getColor();
    assert(std::abs(col[0] - 0.5f) < 1e-5f);
    assert(std::abs(col[1] - 0.6f) < 1e-5f);
    assert(std::abs(col[2] - 0.7f) < 1e-5f);

    // Dimensions mutator/accessor
    bodyPart.setDimensions(glm::vec3(1.5f, 2.5f, 3.5f));
    assert(bodyPart.getDimensions().x == 1.5f);
    assert(bodyPart.getDimensions().y == 2.5f);
    assert(bodyPart.getDimensions().z == 3.5f);

    // Local transform mutator
    glm::mat4 localT = glm::translate(glm::mat4(1.0f), glm::vec3(0.0f, 10.0f, 0.0f));
    bodyPart.setLocalTransform(localT);
    assert(bodyPart.localTransform() == localT);
    assert(bodyPart.getTransform() == localT);

    // setColor PAINTS the primary manifestation through its own Material
    // (it used to only remember the value; updateColor() was never defined).
    const float* painted = bodyPart.getPrimaryObject()->faceColors[0];
    assert(std::abs(painted[0] - 0.5f) < 1e-5f);
    assert(std::abs(painted[1] - 0.6f) < 1e-5f);
    assert(std::abs(painted[2] - 0.7f) < 1e-5f);

    std::cout << "  testAccessorsAndMutators OK\n";
}

static void testPrimaryObjectAndShapes() {
    BodyPart bodyPart("Torso", BodyPart::Type::Torso, ObjectTypes::ShapeKind::Cube, glm::vec3(1.0f));

    // Const and non-const primary object accessors
    Object* primary = bodyPart.getPrimaryObject();
    const BodyPart& constBP = bodyPart;
    const Object* constPrimary = constBP.getPrimaryObject();
    assert(primary != nullptr);
    assert(constPrimary == primary);

    // Primary shape mutation
    assert(bodyPart.getPrimaryObject()->getShapeKind() == ObjectTypes::ShapeKind::Cube);
    bodyPart.setPrimaryShape(ObjectTypes::ShapeKind::Cylinder);
    assert(bodyPart.getPrimaryObject()->getShapeKind() == ObjectTypes::ShapeKind::Cylinder);
    assert(primary->getShapeKind() == ObjectTypes::ShapeKind::Cylinder);

    std::cout << "  testPrimaryObjectAndShapes OK\n";
}

static void testSubObjects() {
    BodyPart bodyPart("Leg", BodyPart::Type::Leg, ObjectTypes::ShapeKind::Cylinder, glm::vec3(1.0f));
    assert(bodyPart.getSubObjectCount() == 0);
    assert(bodyPart.getSubObject(0) == nullptr);
    const BodyPart& constBP = bodyPart;
    assert(constBP.getSubObject(0) == nullptr);

    // Test addSubObject overload 1 (ShapeKind)
    bodyPart.addSubObject(ObjectTypes::ShapeKind::Sphere);
    assert(bodyPart.getSubObjectCount() == 1);

    glm::mat4 offset(1.0f);
    offset = glm::translate(offset, glm::vec3(0.0f, -1.0f, 0.0f));
    bodyPart.addSubObject(ObjectTypes::ShapeKind::Cube, offset);
    assert(bodyPart.getSubObjectCount() == 2);

    const glm::mat4& fetchedOffset = bodyPart.getSubObjectLocalOffset(1);
    assert(fetchedOffset[3][1] == -1.0f); // Check translation Y

    // Out-of-bounds offset lookup returns identity matrix
    const glm::mat4& oobOffset = bodyPart.getSubObjectLocalOffset(99);
    assert(oobOffset == glm::mat4(1.0f));

    // Test addSubObject overload 2 (unique_ptr) with nullptr
    Object* nullResult = bodyPart.addSubObject(nullptr, glm::mat4(1.0f));
    assert(nullResult == nullptr);
    assert(bodyPart.getSubObjectCount() == 2);

    // Test addSubObject overload 2 with valid Object
    auto customSub = std::make_unique<Object>("Leg.customSub");
    customSub->setShape(ObjectTypes::ShapeKind::Cone);
    Object* addedCustom = bodyPart.addSubObject(std::move(customSub), glm::translate(glm::mat4(1.0f), glm::vec3(0.0f, 2.0f, 0.0f)));
    assert(addedCustom != nullptr);
    assert(bodyPart.getSubObjectCount() == 3);
    assert(bodyPart.getSubObject(2) == addedCustom);
    assert(constBP.getSubObject(2) == addedCustom);

    // Test getSubObjects() vector getter
    const auto& subVec = bodyPart.getSubObjects();
    assert(subVec.size() == 3);

    // Test getAllObjects()
    std::vector<Object*> allObjs = bodyPart.getAllObjects();
    // Primary object + 3 sub objects = 4
    assert(allObjs.size() == 4);
    assert(allObjs[0] == bodyPart.getPrimaryObject());
    assert(allObjs[1] == bodyPart.getSubObject(0));
    assert(allObjs[2] == bodyPart.getSubObject(1));
    assert(allObjs[3] == bodyPart.getSubObject(2));

    // Test setSubObjectLocalOffset
    glm::mat4 newOffset = glm::translate(glm::mat4(1.0f), glm::vec3(10.0f, 0.0f, 0.0f));
    bodyPart.setSubObjectLocalOffset(0, newOffset);
    assert(bodyPart.getSubObjectLocalOffset(0) == newOffset);
    // Out-of-bounds setSubObjectLocalOffset (should be no-op safely)
    bodyPart.setSubObjectLocalOffset(99, newOffset);

    // Test removeSubObject
    // Out-of-bounds removeSubObject (should be no-op safely)
    bodyPart.removeSubObject(99);
    assert(bodyPart.getSubObjectCount() == 3);

    // Valid removeSubObject
    bodyPart.removeSubObject(0);
    assert(bodyPart.getSubObjectCount() == 2);
    const glm::mat4& fetchedOffset2 = bodyPart.getSubObjectLocalOffset(0);
    assert(fetchedOffset2[3][1] == -1.0f); // Index shifts down

    std::cout << "  testSubObjects OK\n";
}

static void testTransformPropagation() {
    BodyPart bodyPart("Torso", BodyPart::Type::Torso, ObjectTypes::ShapeKind::Cube, glm::vec3(1.0f));
    bodyPart.addSubObject(ObjectTypes::ShapeKind::Sphere);

    glm::mat4 transform = glm::translate(glm::mat4(1.0f), glm::vec3(5.0f, 0.0f, 0.0f));
    bodyPart.setTransform(transform);

    // Ensure primary object inherited transform
    Object* primary = bodyPart.getPrimaryObject();
    assert(primary != nullptr);
    assert(primary->getTransform()[3][0] == 5.0f);

    // Ensure sub-object inherited transform
    Object* sub = bodyPart.getSubObject(0);
    assert(sub != nullptr);
    assert(sub->getTransform()[3][0] == 5.0f);

    std::cout << "  testTransformPropagation OK\n";
}

static void testAutomations() {
    // Automation clips animate the manifestation; they are the Object's
    // affordance, reached through it — BodyPart no longer forwards them.
    BodyPart bodyPart("Foot", BodyPart::Type::Foot, ObjectTypes::ShapeKind::Cube, glm::vec3(1.0f));
    Object& manifestation = *bodyPart.getPrimaryObject();
    assert(manifestation.hasAutomations() == false);

    Automation::Clip clip;
    clip.name = "walk_step";
    manifestation.addAutomation(clip);
    assert(manifestation.hasAutomations() == true);

    glm::mat4 rest(1.0f);
    manifestation.setAutomationRest(rest);
    glm::mat4 sampled = manifestation.sampleAutomations(rest);
    (void)sampled;

    manifestation.advanceAutomations(0.016f);

    auto& state = manifestation.automationState();
    (void)state;

    manifestation.clearAutomations();
    assert(manifestation.hasAutomations() == false);

    std::cout << "  testAutomations OK\n";
}

static void testDraw() {
    BodyPart bodyPart("Arm", BodyPart::Type::Arm, ObjectTypes::ShapeKind::Cube, glm::vec3(1.0f));
    bodyPart.addSubObject(ObjectTypes::ShapeKind::Sphere);

    // Draw call using MockRenderer
    MockRenderer mockRenderer;
    setCurrentRenderer(&mockRenderer);

    bodyPart.draw();

    assert(mockRenderer.drawMeshCount > 0);

    setCurrentRenderer(nullptr);

    std::cout << "  testDraw OK\n";
}

// BodyPart_Constitution_As_Explicit_Relation (Zach, 2026-10-13): constitution
// is a first-class `constituted-by` Relation, constitution does not imply
// identity, and replacing a manifestation does not change the part.
static void testConstitutionIsExplicitRelation() {
    BodyPart part("LeftHand", BodyPart::Type::Hand, ObjectTypes::ShapeKind::Cube, glm::vec3(1.0f));
    Object* primary = part.getPrimaryObject();

    Relation* edge = part.constitutionOf(primary);
    assert(edge != nullptr);
    assert(edge->type == BodyPart::kConstitutedByType);
    assert(edge->directed);
    assert(edge->a() == &part && edge->b() == primary);
    // A manifestation is not the part: distinct beings, distinct names.
    assert(primary->getIdentifier() != part.getIdentifier());

    // Traversal from the visual Object back to the part, through the
    // graph's own endpoint index (what the `Related` condition uses).
    std::vector<Relation*> involving;
    part.relations().relationsInvolving(*primary, involving);
    bool foundPart = false;
    for (Relation* rel : involving)
        if (rel && rel->type == BodyPart::kConstitutedByType && rel->a() == &part) foundPart = true;
    assert(foundPart);

    // Replace a manifestation: the part's identity is untouched, the old
    // edge dissolves, the new one forms.
    const std::string identity = part.getIdentifier();
    Object* first = part.addSubObject(ObjectTypes::ShapeKind::Sphere);
    assert(part.constitutionOf(first) != nullptr);
    part.removeSubObject(0);
    assert(part.constitutionOf(first) == nullptr);
    Object* second = part.addSubObject(ObjectTypes::ShapeKind::Cylinder);
    assert(part.constitutionOf(second) != nullptr);
    assert(part.getIdentifier() == identity);

    // Registered, not hidden (Refusal 6): the constitution reads as a path.
    auto* constitutedBy = part.findProperty("constitutedBy");
    assert(constitutedBy != nullptr);
    auto list = std::get<std::shared_ptr<PropertyList>>(constitutedBy->value());
    assert(list && list->elements.size() == 2);   // primary + the replacement
    auto* primaryPath = part.findProperty("primaryObject");
    assert(primaryPath != nullptr);
    assert(std::get<std::string>(primaryPath->value()) == primary->getIdentifier());

    std::cout << "  testConstitutionIsExplicitRelation OK\n";
}

int main() {
    std::cout << "body_part_test:\n";
    testInitialState();
    testCustomInitialization();
    testAccessorsAndMutators();
    testPrimaryObjectAndShapes();
    testSubObjects();
    testTransformPropagation();
    testAutomations();
    testDraw();
    testConstitutionIsExplicitRelation();
    std::cout << "body_part_test: ALL OK\n";
    return 0;
}
