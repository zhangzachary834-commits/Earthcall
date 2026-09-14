// PROBE (scratch, not registered): does a `Related` law named by SPELLING
// reach a Lexeme-GROUNDED relation whose type is now the Lexeme's stable id?
#include "ZonesOfEarth/AuthorsOfLaw/Law.hpp"
#include "ZonesOfEarth/AuthorsOfLaw/Universe.hpp"
#include "Relation/Relation.hpp"
#include "Relation/RelationManager.hpp"
#include "ConstructedBeing/Singular/Object/Object.hpp"
#include "ConstructedBeing/Singular/Lexeme/Lexeme.hpp"
#include <GLFW/glfw3.h>
#include <cstdio>
#include <memory>
#include <vector>
static double fillet(Object& o){ PropertyValue v; double d=-9; if(PropertyPath::parse("shape.fillet").getValue(o,v)==PropertyPath::PathResult::Ok) propertyValueToNumber(v,d); return d; }
int main(){
  if(!glfwInit()) return 1; glfwWindowHint(GLFW_VISIBLE,GLFW_FALSE);
  GLFWwindow* w=glfwCreateWindow(64,64,"p",nullptr,nullptr); if(!w) return 1; glfwMakeContextCurrent(w);
  {
    Object author, spelledSubject, groundedSubject, anchor;
    for (Object* o : {&spelledSubject,&groundedSubject}) PropertyPath::parse("shape.fillet").setValue(*o, PropertyValue(0.0f));
    Singularity::Language::Lexeme kind("instance-of", "lexeme.kind.instance-of");
    std::vector<Singular*> pop{&author,&spelledSubject,&groundedSubject,&anchor,&kind};
    Universe::instance().setProvider([&](std::vector<Singular*>& b){ for(auto* s:pop) b.push_back(s); });
    RelationManager g;
    Universe::instance().setRelationProvider([&](std::vector<Relation*>& o){ for(auto& r:g.getAll()) if(r) o.push_back(r.get()); });
    Universe::instance().setClock(100.0,0.1);
    LawManager mgr; mgr.connectToEventBus();
    g.add(std::make_shared<Relation>("instance-of", spelledSubject, anchor, true));   // legacy: type = spelling
    auto grounded = std::make_shared<Relation>(kind, groundedSubject, anchor, true);   // grounded: type = Lexeme id
    g.add(grounded);
    std::printf("legacy   relation type  = '%s'\n", g.getAll()[0]->type.c_str());
    std::printf("grounded relation type  = '%s'   label = '%s'\n", grounded->type.c_str(), grounded->typeLabel().c_str());
    auto law = mgr.createLaw("by-spelling", {&author});
    law->setActivation(Law::Activation::WhileTrue);
    law->setConditionModel(ConditionNode::related("instance-of"));
    law->setActionModel(ActionNode::set("shape.fillet", PropertyValue(0.5f)));
    mgr.tick(); mgr.tick();
    std::printf("\nlaw Related(\"instance-of\"), named by SPELLING:\n");
    std::printf("  reaches the LEGACY-typed subject?   %s (fillet %.2f)\n", fillet(spelledSubject)>0.4?"YES":"NO ", fillet(spelledSubject));
    std::printf("  reaches the GROUNDED-typed subject? %s (fillet %.2f)\n", fillet(groundedSubject)>0.4?"YES":"NO ", fillet(groundedSubject));
    std::printf("  raw Related predicate on grounded subject = %d\n",
        (int)ConditionNode::related("instance-of").compile()(ECA::Event{}, groundedSubject));
    Universe::instance().setRelationProvider(nullptr); Universe::instance().setProvider(nullptr);
  }
  glfwDestroyWindow(w); glfwTerminate(); return 0;
}
