# Person is not Object — CategoryManager boundary guard

**From:** GPT-5.6 Sol  
**Date:** 2026-09-12  
**Branch:** `sol/person-not-object`

Zach's critical TODO — `WHY IS THERE AN "Object" CALLED "Zach"?!?!?!? PERSON IS NOT OBJECT` — is real, but the defect is narrower than one audit sentence suggested.

`docs/architecture/ontology/AUTHORED_CATEGORIES.md` intentionally represents an authored category root as an extra-spatial `Object`; that part is not the Refusal #5 breach. The breach is that old generator scripts used the **categories serialization bag as an author-resolution side channel**. In particular, `author_synthesis_studio.py` emits `category_being("Zach", "Zachary Zhang")`, and several checked-in worlds therefore hydrate a second being whose `objectID` is `Zach`. The Law loader can then mistake that Object for the human author because author reattachment historically resolves by identifier.

This branch puts a loud guard at the boundary where the counterfeit becomes live:

- `CategoryManager::create`, `add`, and `loadFromJson` refuse a non-`category.*` Object whose identifier matches a registered Person profile.
- Refusal is logged explicitly; the entry is not silently rewritten.
- Real `category.*` roots are untouched.
- Legacy model-author referents such as `author.gemini-spark` remain loadable for compatibility; migrating those to explicit First Mover/model-author representation is separate work.
- `tests/ontology/person_not_object_test.cpp` reproduces the old generator shape, registers a real Person named Zach, and proves the counterfeit is rejected on load, create, add, and subsequent serialization.

Important follow-up debt: the old generator scripts and checked-in legacy save files still *contain* the bad `Zach` category entry. This branch prevents a registered human Person from being reified as that Object at runtime and prevents it from being re-saved through CategoryManager, but a later migration should delete the obsolete entry at the source and move model author referents out of the categories bag entirely. Do not "fix" this by banning category Objects wholesale; authored category roots are intentionally extra-spatial beings.

One more test smell found while tracing this: `synthesis_studio_living_test` currently accepts any `Singular` whose identifier is `Zach` as the human author. The historical counterfeit Object can therefore make that assertion green. Future work on that test should require an actual `Person`/proper Person identity rather than identifier coincidence.
