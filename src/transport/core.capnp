@0xd27081948f8c0117;

annotation nameinfix @0x85a8d86d736ba637 (file): Text;
annotation fieldgetset @0xf72bc690355d66de (file): Void;
$fieldgetset;

struct LogEntry
{
    term @0 :UInt64;
    id @1 :UInt64;
    type @2 :UInt64;
    data @3 :Data;
}

struct RequestVoteMsg
{
    term @0 :UInt64;
    candidateId @1 :UInt64;
    lastLogIdx @2 :UInt64;
    lastLogTerm @3 :UInt64;
}

struct RequestVoteResponseMsg
{
    term @0 :UInt64;
    voteGranted @1 :UInt64;
}

struct AppendEntriesMsg
{
    term @0 :UInt64;
    prevLogIdx @1 :UInt64;
    prevLogTerm @2 :UInt64;
    leaderCommit @3 :UInt64;
    nEntries @4 :UInt64;
    entries @5 :List(LogEntry);
}

struct AppendEntriesResponseMsg
{
    term @0 :UInt64;
    success @1 :UInt64;
    currentIdx @2 :UInt64;
    firstIdx @3 :UInt64;
}
