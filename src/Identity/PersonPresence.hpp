#pragma once

#include <string>

class Person;

namespace Identity {

// ---------------------------------------------------------------------------
// The Person becomes PRESENT: they prove possession of their key to this
// process, and the First Mover Register seats them as its trusted root.
//
// Two doors reach this, and they must behave identically:
//   * boot, with EARTHCALL_KEY_PASSPHRASE (EngineInit), and
//   * the Terminal's Identity Zone, with a passphrase typed as a secret line
//     (TerminalChannel; Terminal_Zones.md, Zach 2026-09-30).
//
// The passphrase is taken by const reference and never stored, logged, or
// placed in any property: it is state beneath the Kernel (NO_BLACK_BOX §5).
// ---------------------------------------------------------------------------
struct PresenceResult {
    bool ok = false;
    std::string report;   // one line for the Person; never contains the secret
};

// A keyed Person's profile is admitted only when its KeyStore key opens with
// this passphrase and matches (the single keyed profile in saves/persons, or
// the one EARTHCALL_PERSON_ID names). If `person` already has an identity it
// is simply unlocked. On success the register trusts the Person and they log in.
PresenceResult unlockPresentPerson(Person& person, const std::string& passphrase);

// FIRST keying: an explicit trust act, never a load side effect. Mints the
// Person's key, seals it under the passphrase, records the migration ledger,
// saves the profile, and makes them present. Refuses if they already have a key.
PresenceResult keyPresentPerson(Person& person, const std::string& passphrase);

// Is there a keyed profile on disk the present Person could unlock into?
bool keyedProfileExists();

} // namespace Identity
