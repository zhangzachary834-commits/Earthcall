    ecNeed(outPath,"output directory argument");const std::string dir=outPath;
    std::vector<ECTransition> states;
    auto add=[&](const char* name){ECTransition s;s.name=name;states.push_back(s);return &states.back();};
    add("baseline");add("baseline-repeat");add("toggle-off")->on=false;add("toggle-on");
    add("independent-clocks")->clocks={7.25,13.5,-2.0,4.75};add("clock-recover");
    add("emission-edit")->fixture="emission";add("emission-recover");
    add("density-edit")->fixture="density";add("density-recover");
    add("unsupported-structure")->fixture="unsupported";add("structure-recover");
    add("move-and-extent")->fixture="bounds";add("bounds-recover");
    add("membership-three")->members=3;add("membership-recover");
    add("positive-source")->source=1;add("source-recover");
    auto* no=add("absent-source-nonwi");no->source=2;no->fixture="nonwi";add("phase-source-recover");
    auto* odd=add("resize-odd");odd->w=321;odd->h=181;
    auto* wide=add("resize-wide");wide->w=640;wide->h=360;add("resize-recover");
    add("opaque-front")->opaque=true;add("opaque-recover");
    add("forced-refusal")->force=true;add("refusal-recover");
    add("camera-outside")->outside=true;add("camera-recover");
    add("fresh-device")->recreate=true;add("fresh-device-repeat");
    auto savedDensity=ecCopy(*active[0]->volumeDensity),savedEmission=ecCopy(*active[0]->volumeEmission);std::vector<OntoMath::Piecewise> savedPhases;for(auto* f:active)savedPhases.push_back(ecCopy(*f->volumePhase));
    const auto savedOrigin=active[0]->origin,savedScale=active[0]->scale;std::string fixture="base";
    auto applyFixture=[&](const std::string& name){if(fixture==name)return;
        *active[0]->volumeDensity=ecCopy(savedDensity);*active[0]->volumeEmission=ecCopy(savedEmission);active[0]->origin=savedOrigin;active[0]->scale=savedScale;
        for(size_t i=0;i<active.size();++i){*active[i]->volumePhase=ecCopy(savedPhases[i]);active[i]->noteAuthoredChannelWritten(geom::FieldNode::kMathVolumePhase);}
        if(name=="emission")ecMultiply(*active[0]->volumeEmission,.4);
        if(name=="density"){auto& n=*active[0]->volumeDensity->pieces[0].mathNode;ecNeed(n.op==OntoMath::MathNode::Op::Scale,"authored density shape");n.children[0]->scalarForm=OntoMath::ScalarForm::constant(0);}
        if(name=="unsupported"){auto& x=active[0]->volumeDensity->pieces[0];auto n=std::make_shared<OntoMath::MathNode>();n->op=OntoMath::MathNode::Op::Add;n->children.push_back(std::make_unique<OntoMath::MathNode>(*x.mathNode));n->children.push_back(std::make_unique<OntoMath::MathNode>(*ecConstant(0).pieces[0].mathNode));x.mathNode=n;}
        if(name=="bounds"){active[0]->origin.x+=8;active[0]->scale.z*=.7f;}
        if(name=="nonwi")for(auto* f:active){*f->volumePhase=ecConstant(1);f->noteAuthoredChannelWritten(geom::FieldNode::kMathVolumePhase);}
        active[0]->noteAuthoredChannelWritten(geom::FieldNode::kMathVolumeDensity);active[0]->noteAuthoredChannelWritten(geom::FieldNode::kMathVolumeEmission);fixture=name;
    };
    wgpu::Device firstDevice;ecNeed(firstDevice.init(),"native device");wgpu::Device secondDevice;wgpu::Device* gpu=&firstDevice;
    WebGpuRenderer warm;ecNeed(warm.init(*gpu),"warm renderer init");
    Rendering::RadianceSourceBinding source;
    source.producerId = root->getIdentifier();
    source.position = light.position;
    source.ambientRadiance = Rendering::lightAmbientRadiance(light);
    source.diffuseRadiance = Rendering::lightDiffuseRadiance(light);
    source.specularRadiance = Rendering::lightSpecularRadiance(light);
    source.coefficients = glm::vec4(light.intensity, light.ambient, light.diffuse, light.specular);
    source.enabled = light.enabled;
    source.temporalCoordinate = fixedT;
    source.temporalDelta = 1.0 / 60.0;
    // Mirror EngineRender's authored source projection. The pre-2026-10-21
    // fixture omitted these expressions and exercised compatibility lighting;
    // preserve that workload only as an explicitly labelled counterfactual.
    const char* sourceMode = std::getenv("EC_VOLUME_SOURCE_MODE");
    const bool compatibilitySource = sourceMode && std::strcmp(sourceMode, "compatibility") == 0;
    REQUIRE(!sourceMode || compatibilitySource || std::strcmp(sourceMode, "authored") == 0);
    if (!compatibilitySource) {
        root->verifyAuthoredMath();
        if (root->field && root->field->mode == OntoMath::ScalarField::EvaluationMode::AST &&
            !root->field->astDefinition.pieces.empty()) {
            source.radianceExpr = &root->field->astDefinition;
            source.radianceRevision = root->authoredMathRevision(geom::FieldNode::kMathFieldAst);
        }
        if (root->lightChroma && !root->lightChroma->pieces.empty()) {
            source.chromaExpr = root->lightChroma.get();
            source.chromaRevision = root->authoredMathRevision(geom::FieldNode::kMathLightChroma);
        }
        if (root->lightAngular && !root->lightAngular->pieces.empty()) {
            source.angularExpr = root->lightAngular.get();
            source.angularRevision = root->authoredMathRevision(geom::FieldNode::kMathLightAngular);
        }
    }
    std::printf("SOURCE mode=%s radiance=%d chroma=%d angular=%d clock=%.9f\n",
        compatibilitySource ? "compatibility-counterfactual" : "authored",
        source.radianceExpr != nullptr, source.chromaExpr != nullptr,
        source.angularExpr != nullptr, source.temporalCoordinate);




    const auto one=ecConstant(1);ECJson records=ECJson::array();std::vector<uint8_t> baseline;
    const char* flag=std::getenv("EC_6EV_MUTATION");const std::string mutation=flag?flag:"";
    if(!mutation.empty())states.resize(1);
    for(size_t index=0;index<states.size();++index){const auto& state=states[index];applyFixture(state.fixture);
        if(state.recreate){warm.shutdown();ecNeed(secondDevice.init(),"replacement device");gpu=&secondDevice;ecNeed(warm.init(*gpu),"renderer replacement init");}
        std::printf("STATE %zu %s device=%d\n",index,state.name.c_str(),gpu==&secondDevice?1:0);
        auto first=ecFrame(warm,*gpu,state,active,source,one,mutation=="missing-publication"?mutation:"");
        WebGpuRenderer fresh;ecNeed(fresh.init(*gpu),"fresh comparison init");if(state.force){auto prime=state;prime.force=false;ecFrame(fresh,*gpu,prime,active,source,one);}auto expected=ecFrame(fresh,*gpu,state,active,source,one);
        auto bytes=ecPoison(warm,*gpu);auto repeated=ecFrame(warm,*gpu,state,active,source,one,mutation);
        auto save=[&](const ECImage& image,const std::string& label){auto name=dir+"/"+state.name+"-"+label;ecSave(name+".rgba",image.rgba.data(),image.rgba.size());if(!image.publication.empty())ecSave(name+".publication.bin",image.publication.data(),image.publication.size()*4);};
        save(first,"first");save(repeated,"poisoned");save(expected,"fresh");
        auto census=ecCensus(repeated);const bool routed=state.on&&state.members==4&&state.source==0&&state.fixture!="unsupported";
        ECJson row={{"name",state.name},{"fixture",state.fixture},{"width",state.w},{"height",state.h},{"clocks",state.clocks},{"on",state.on},{"force",state.force},{"opaque",state.opaque},{"outside",state.outside},{"members",state.members},{"source",state.source},{"device",gpu==&secondDevice?1:0},{"poison_bytes",bytes},{"expected_routed",routed},{"first_stats",first.stats},{"poisoned_stats",repeated.stats},{"fresh_stats",expected.stats},{"publication",census}};
        row["Screen_first_equals_fresh"]=first.rgba==expected.rgba;row["Screen_poisoned_equals_fresh"]=repeated.rgba==expected.rgba;
        row["publication_first_equals_fresh"]=first.publication==expected.publication;row["publication_poisoned_equals_fresh"]=repeated.publication==expected.publication;
        records.push_back(row);std::ofstream(dir+"/states.json")<<records.dump(2)<<"\n";
        ecNeed(unsigned(repeated.stats["routed_sets"])==unsigned(routed),"intended eligibility "+state.name);
        ecNeed(unsigned(census["nonfinite"])==0,"publication finite "+state.name);
        ecNeed(row["Screen_first_equals_fresh"]&&row["Screen_poisoned_equals_fresh"],"Screen stale or missing output "+state.name);
        ecNeed(row["publication_first_equals_fresh"]&&row["publication_poisoned_equals_fresh"],"publication stale or missing output "+state.name);
        if(routed)ecNeed(repeated.publication.size()==size_t(state.w)*state.h*4&&bytes>0,"poisoned full publication "+state.name);
        if(state.force){auto currentState=state;currentState.on=false;auto current=ecFrame(fresh,*gpu,currentState,active,source,one);save(current,"current");ecNeed(current.rgba==repeated.rgba,"actual Current forced fallback");}
        if(state.name=="baseline")baseline=expected.rgba;
        if(state.name=="independent-clocks"||state.name=="emission-edit"||state.name=="density-edit"||state.name=="move-and-extent")ecNeed(baseline!=expected.rgba,"positive edit sensitivity "+state.name);
        if(state.name=="absent-source-nonwi")ecNeed(unsigned(repeated.stats["program_refusals"])==0,"legitimate no-source compatibility scene");
        fresh.shutdown();std::printf("STATE PASS %s poison=%llu\n",state.name.c_str(),(unsigned long long)bytes);
    }
    warm.shutdown();setCurrentRenderer(nullptr);
    auto release=[](wgpu::Device& d){if(d.queue)wgpuQueueRelease(d.queue);if(d.device)wgpuDeviceRelease(d.device);if(d.adapter)wgpuAdapterRelease(d.adapter);if(d.instance)wgpuInstanceRelease(d.instance);};release(secondDevice);release(firstDevice);
    std::printf("TRANSITION PASS states=%zu mutation=%s; state validity only, no accuracy/FPS claim\n",records.size(),mutation.c_str());return 0;
}catch(const std::exception& e){std::fprintf(stderr,"TRANSITION FAIL %s\n",e.what());return 42;}
}
