# When Bounds Are Doctrine And When They Are Not

Zach explicitly directed in the to-do list: "we need the ability to author maximum chain depth per tick of chaining laws. currently we just resolve that with 'kmaxbound' and 'kmaxchain' or something. One of the previous objections to this is that this would break law behavior across differnt devices/zones. However, if it's an authorable property, these maxchains can be enforced by Metalaws in Zone jurisdictions to be universal. The remaining tradeoff is it'll take effort to work around differnt ones, but that's doable and not unfeasible. The goal is to expose and give granualr control over the machine as much as we can in a similar philosophy to cpp itself."

The previous agents internalized the phrase "bounds are doctrine, not limits" from `ENGINEERING_DISCIPLINE.md` (and quoted it frequently in `The_Second_Person_and_the_Speed_of_Frameworks.md`). They treated `kMaxChainRounds` as an inviolable structural limit that protected Earthcall from run-away computational explosion, similar to an anti-Babel ceiling.

However, Zach's ruling clarifies that "bounds are doctrine" applies to the necessity of having bounds, but the *value* of the bound does not need to be hardcoded in the C++ kernel. A hardcoded bound is a black box limit that hides the machine from the Person.

By exposing `maxChainRounds` as a property of the LawManager, we allow Zone jurisdictions and Metalaws to enforce bounds universally, bringing them into the realm of authored law rather than compiled constraints. The bound is still doctrine—a law that cascades endlessly is still a badly formed law—but the choice of *where* that ceiling sits is returned to the author.

This aligns perfectly with the manifesto's core philosophy: "give granular control over the machine as much as we can in a similar philosophy to cpp itself." The C++ layer provides the mechanism (the enforcement of the limit), but the authored world dictates the policy (the value of the limit).

- Jules, 2024-05-18
