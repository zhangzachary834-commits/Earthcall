        // Rung 4 Migration: Seed Laws for Rendering Optimization
        // These laws displace the old C++ Engine decision of "if exact supported, do analytic".
        // They only operate when renderMode is Auto (0), honoring explicit Person authored modes.
        
        // Law 1: Auto -> Analytic (when supported)
        auto renderOptAnalytic = std::make_shared<FirstMoverLaw>("Renderer Optimization: Analytic Path");
        renderOptAnalytic->setIdentifier("seed.renderer.opt.analytic");
        renderOptAnalytic->setScope(Law::Scope::Everyone);
        renderOptAnalytic->setActivation(Law::Activation::WhileTrue);
        
        ConditionNode exactSupported = ConditionNode::compare("screen.rendersImplicitExactly", ConditionNode::Op::Eq, true);
        ConditionNode isAutoMode1 = ConditionNode::compare("renderMode", ConditionNode::Op::Eq, 0); // RenderMode::Auto
        
        ConditionNode allAnalytic;
        allAnalytic.kind = ConditionNode::Kind::All;
        allAnalytic.children.push_back(exactSupported);
        allAnalytic.children.push_back(isAutoMode1);
        renderOptAnalytic->setConditionModel(allAnalytic);
        
        renderOptAnalytic->setActionModel(ActionNode::set("renderMode", 1)); // RenderMode::Analytic
        laws.add(renderOptAnalytic);
        
        // Law 2: Auto -> Mesh (when analytic not supported)
        auto renderOptMesh = std::make_shared<FirstMoverLaw>("Renderer Optimization: Mesh Fallback");
        renderOptMesh->setIdentifier("seed.renderer.opt.mesh");
        renderOptMesh->setScope(Law::Scope::Everyone);
        renderOptMesh->setActivation(Law::Activation::WhileTrue);
        
        ConditionNode exactUnsupported = ConditionNode::compare("screen.rendersImplicitExactly", ConditionNode::Op::Eq, false);
        ConditionNode isAutoMode2 = ConditionNode::compare("renderMode", ConditionNode::Op::Eq, 0); // RenderMode::Auto
        
        ConditionNode allMesh;
        allMesh.kind = ConditionNode::Kind::All;
        allMesh.children.push_back(exactUnsupported);
        allMesh.children.push_back(isAutoMode2);
        renderOptMesh->setConditionModel(allMesh);
        
        renderOptMesh->setActionModel(ActionNode::set("renderMode", 2)); // RenderMode::Mesh
        laws.add(renderOptMesh);
