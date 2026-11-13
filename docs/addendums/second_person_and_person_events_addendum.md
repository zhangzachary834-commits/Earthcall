# Second Person Framework and Person events

**Companions:** [Second Person Framework](../architecture/ourverse/SECOND_PERSON_FRAMEWORK.md) and [Person Events System](../architecture/events/PERSON_EVENTS_SYSTEM.md).

Person lifecycle events can notify subscribers that a Person was created, logged in, logged out, or joined a Zone. The concrete zone-join event is `PersonJoinedEvent` (not `PersonZoneJoinEvent`); `PersonLoginEvent` is the login notification. These signals provide useful occasions to update observers and evaluate relevant Laws.

**Notification is not authorization.** The Second Person Framework specifies Person-to-Person visibility, representation, and conflict boundaries, and explicitly says those three mechanisms remain design work. Receiving a login or join event does not by itself enforce disclosure, consent, jurisdiction, or constitutional Person protections. Those require their own correctly grounded Relation/TransferPolicy and Kernel enforcement paths, including author-reserved decisions.

**Design consequence:** use events as observations or triggers where appropriate, but never describe their existence as proof that the Second Person Framework's proposed gates have been implemented.
