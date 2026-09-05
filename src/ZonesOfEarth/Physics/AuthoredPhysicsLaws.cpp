#include "AuthoredPhysicsLaws.hpp"
#include "ZonesOfEarth/AuthorsOfLaw/ActionModel.hpp"
#include "Singularity/OntoMath/CurveModel.hpp"
#include "Singularity/OntoMath/ScalarForm.hpp"

namespace Physics {

std::vector<std::shared_ptr<Law>> createAuthoredRotationalLaws() {
    std::vector<std::shared_ptr<Law>> laws;

    // -----------------------------------------------------------------------
    // Law: law-angular-kinematics
    // Flow action over rotation: rotation := rotation + angularVelocity * dt
    // Expressed directly in degrees and degrees/second.
    // -----------------------------------------------------------------------
    {
        auto rotKinematics = std::make_shared<Law>("physics: angular kinematics");
        rotKinematics->setLawIdentifier("law-angular-kinematics");
        rotKinematics->setActivation(Law::Activation::WhileTrue);
        rotKinematics->setScope(Law::Scope::Everyone);

        auto omegaNode = std::make_shared<OntoMath::MathNode>();
        omegaNode->op = OntoMath::MathNode::Op::ValueLeaf;
        omegaNode->variableName = "omega";

        MathBindings rotBindings;
        rotBindings["omega"] = PropertyPath::parse("angularVelocity");

        rotKinematics->setActionModel(ActionNode::flow(
            "rotation", OntoMath::Piecewise::continuous(omegaNode), rotBindings));
        laws.push_back(rotKinematics);
    }

    // -----------------------------------------------------------------------
    // Law: law-angular-damping
    // Atmospheric and mechanical friction: angularVelocity := angularVelocity - gamma * angularVelocity * dt
    // -----------------------------------------------------------------------
    {
        auto dampLaw = std::make_shared<Law>("physics: angular damping");
        dampLaw->setLawIdentifier("law-angular-damping");
        dampLaw->setActivation(Law::Activation::WhileTrue);
        dampLaw->setScope(Law::Scope::Everyone);

        auto dampOmega = std::make_unique<OntoMath::MathNode>();
        dampOmega->op = OntoMath::MathNode::Op::ValueLeaf;
        dampOmega->variableName = "omega";

        auto scaleNode = std::make_shared<OntoMath::MathNode>();
        scaleNode->op = OntoMath::MathNode::Op::Scale;
        scaleNode->children.push_back(std::move(dampOmega));

        auto dragConst = std::make_unique<OntoMath::MathNode>();
        dragConst->op = OntoMath::MathNode::Op::ScalarLeaf;
        dragConst->scalarForm = OntoMath::ScalarForm::constant(-0.5);
        scaleNode->children.push_back(std::move(dragConst));

        MathBindings dampBindings;
        dampBindings["omega"] = PropertyPath::parse("angularVelocity");

        dampLaw->setActionModel(ActionNode::flow(
            "angularVelocity", OntoMath::Piecewise::continuous(scaleNode), dampBindings));
        laws.push_back(dampLaw);
    }

    // -----------------------------------------------------------------------
    // Law: law-gravity-tilt
    // Off-center center of mass generates gravitational torque:
    // tau = centerOfMass x (m * g)
    // alpha = (tau / I) * (180 / pi)
    // angularVelocity := angularVelocity + alpha * dt
    // -----------------------------------------------------------------------
    {
        auto tiltLaw = std::make_shared<Law>("physics: gravity tilt");
        tiltLaw->setLawIdentifier("law-gravity-tilt");
        tiltLaw->setActivation(Law::Activation::WhileTrue);
        tiltLaw->setScope(Law::Scope::Everyone);

        auto crossNode = std::make_unique<OntoMath::MathNode>();
        crossNode->op = OntoMath::MathNode::Op::Cross;

        auto comNode = std::make_unique<OntoMath::MathNode>();
        comNode->op = OntoMath::MathNode::Op::ValueLeaf;
        comNode->variableName = "com";

        auto gNode = std::make_unique<OntoMath::MathNode>();
        gNode->op = OntoMath::MathNode::Op::VectorConstruct;
        auto c0 = std::make_unique<OntoMath::MathNode>();
        c0->op = OntoMath::MathNode::Op::ScalarLeaf;
        c0->scalarForm = OntoMath::ScalarForm::constant(0.0);
        auto cG = std::make_unique<OntoMath::MathNode>();
        cG->op = OntoMath::MathNode::Op::ScalarLeaf;
        cG->scalarForm = OntoMath::ScalarForm::constant(-9.81);
        auto cZ = std::make_unique<OntoMath::MathNode>();
        cZ->op = OntoMath::MathNode::Op::ScalarLeaf;
        cZ->scalarForm = OntoMath::ScalarForm::constant(0.0);
        gNode->children.push_back(std::move(c0));
        gNode->children.push_back(std::move(cG));
        gNode->children.push_back(std::move(cZ));

        crossNode->children.push_back(std::move(comNode));
        crossNode->children.push_back(std::move(gNode));

        auto alphaNode = std::make_shared<OntoMath::MathNode>();
        alphaNode->op = OntoMath::MathNode::Op::Scale;
        alphaNode->children.push_back(std::move(crossNode));

        // Constant conversion (180/pi / nominal_inertia)
        auto degScale = std::make_unique<OntoMath::MathNode>();
        degScale->op = OntoMath::MathNode::Op::ScalarLeaf;
        degScale->scalarForm = OntoMath::ScalarForm::constant(229.183118);
        alphaNode->children.push_back(std::move(degScale));

        MathBindings tiltBindings;
        tiltBindings["com"] = PropertyPath::parse("centerOfMass");

        tiltLaw->setActionModel(ActionNode::flow(
            "angularVelocity", OntoMath::Piecewise::continuous(alphaNode), tiltBindings));
        laws.push_back(tiltLaw);
    }

    // -----------------------------------------------------------------------
    // Law: law-rolling-coupling
    // Surface contact couples linear forward velocity into rotational roll:
    // omega_roll = (n_ground x v) / R * (180 / pi)
    // Maps linear velocity to roll rate around transverse axis.
    // -----------------------------------------------------------------------
    {
        auto rollLaw = std::make_shared<Law>("physics: rolling coupling");
        rollLaw->setLawIdentifier("law-rolling-coupling");
        rollLaw->setActivation(Law::Activation::WhileTrue);
        rollLaw->setScope(Law::Scope::Everyone);

        auto rollNode = std::make_shared<OntoMath::MathNode>();
        rollNode->op = OntoMath::MathNode::Op::VectorConstruct;

        auto rollX = std::make_unique<OntoMath::MathNode>();
        rollX->op = OntoMath::MathNode::Op::ScalarLeaf;
        rollX->scalarForm = OntoMath::ScalarForm::variable("vz", 1.0, 114.591559);

        auto rollY = std::make_unique<OntoMath::MathNode>();
        rollY->op = OntoMath::MathNode::Op::ScalarLeaf;
        rollY->scalarForm = OntoMath::ScalarForm::constant(0.0);

        auto rollZ = std::make_unique<OntoMath::MathNode>();
        rollZ->op = OntoMath::MathNode::Op::ScalarLeaf;
        rollZ->scalarForm = OntoMath::ScalarForm::variable("vx", 1.0, -114.591559);

        rollNode->children.push_back(std::move(rollX));
        rollNode->children.push_back(std::move(rollY));
        rollNode->children.push_back(std::move(rollZ));

        MathBindings rollBindings;
        rollBindings["vx"] = PropertyPath::parse("velocity.x");
        rollBindings["vz"] = PropertyPath::parse("velocity.z");

        rollLaw->setActionModel(ActionNode::map(
            "angularVelocity", OntoMath::Piecewise::continuous(rollNode), rollBindings));
        laws.push_back(rollLaw);
    }

    return laws;
}

} // namespace Physics
