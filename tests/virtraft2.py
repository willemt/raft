#!/usr/bin/env python
"""
virtraft2 - Simulate a raft network

Some quality checks we do:
 - Log Matching (servers must have matching logs)
 - State Machine Safety (applied entries have the same ID)
 - Election Safety (only one valid leader per term)
 - Current Index Validity (does current index have an existing entry?)
 - Entry ID Monotonicity (entries aren't appended out of order)
 - Committed entry popping (committed entries are not popped from the log)
 - Log Accuracy (does the server's log match mirror an independent log?)
 - Deadlock detection (does the cluster continuously make progress?)

Some chaos we generate:
 - Random bi-directional partitions between nodes
 - Message dropping
 - Message duplication
 - Message re-ordering

Usage:
  virtraft --servers SERVERS [-d RATE] [-D RATE] [-c RATE] [-C RATE] [-m RATE]
                             [-P RATE] [-s SEED] [-i ITERS] [-p] [--tsv]
                             [-q] [-v] [-l LEVEL]
  virtraft --version
  virtraft --help

Options:
  -n --servers SERVERS       Number of servers
  -d --drop_rate RATE        Message drop rate 0-100 [default: 0]
  -D --dupe_rate RATE        Message duplication rate 0-100 [default: 0]
  -c --client_rate RATE      Rate entries are received from the client 0-100
                             [default: 100]
  -C --compaction_rate RATE  Rate that log compactions occur 0-100 [default: 0]
  -m --member_rate RATE      Membership change rate 0-100000 [default: 0]
  -P --partition_rate RATE   Rate that partitions occur or are healed 0-100
                             [default: 0]
  -p --no_random_period      Don't use a random period
  -s --seed SEED             The simulation's seed [default: 0]
  -q --quiet                 No output at end of run
  -i --iterations ITERS      Number of iterations before the simulation ends
                             [default: 1000]
  --tsv                      Output node status tab separated values at exit
  -v --verbose               Show debug logs
  -l --log_level LEVEL       Set log level
  -V --version               Display version.
  -h --help                  Prints a short usage summary.

Examples:

  Output a node status table:
    virtraft --servers 3 --iterations 1000 --tsv | column -t
"""
import collections
import colorama
import coloredlogs
import docopt
import logging
import random
import re
import sys
import terminaltables

from raft_cffi import ffi, lib

NODE_DISCONNECTED = 0
NODE_CONNECTING = 1
NODE_CONNECTED = 2
NODE_DISCONNECTING = 3


class ServerDoesNotExist(Exception):
    pass


def logtype2str(log_type):
    if log_type == lib.RAFT_LOGTYPE_NORMAL:
        return 'normal'
    elif log_type == lib.RAFT_LOGTYPE_DEMOTE_NODE:
        return 'demote'
    elif log_type == lib.RAFT_LOGTYPE_REMOVE_NODE:
        return 'remove'
    elif log_type == lib.RAFT_LOGTYPE_ADD_NONVOTING_NODE:
        return 'add_nonvoting'
    elif log_type == lib.RAFT_LOGTYPE_ADD_NODE:
        return 'add'
    else:
        return 'unknown'


def state2str(state):
    if state == lib.RAFT_STATE_LEADER:
        return colorama.Fore.GREEN + 'leader' + colorama.Style.RESET_ALL
    elif state == lib.RAFT_STATE_CANDIDATE:
        return 'candidate'
    elif state == lib.RAFT_STATE_FOLLOWER:
        return 'follower'
    else:
        return 'unknown'


def connectstatus2str(connectstatus):
    return {
        NODE_DISCONNECTED: colorama.Fore.RED + 'DISCONNECTED' + colorama.Style.RESET_ALL,
        NODE_CONNECTING: 'CONNECTING',
        NODE_CONNECTED: colorama.Fore.GREEN + 'CONNECTED' + colorama.Style.RESET_ALL,
        NODE_DISCONNECTING: colorama.Fore.YELLOW + 'DISCONNECTING' + colorama.Style.RESET_ALL,
    }[connectstatus]


def err2str(err):
    return {
        lib.RAFT_ERR_NOT_LEADER: 'RAFT_ERR_NOT_LEADER',
        lib.RAFT_ERR_ONE_VOTING_CHANGE_ONLY: 'RAFT_ERR_ONE_VOTING_CHANGE_ONLY',
        lib.RAFT_ERR_SHUTDOWN: 'RAFT_ERR_SHUTDOWN',
        lib.RAFT_ERR_NOMEM: 'RAFT_ERR_NOMEM',
        lib.RAFT_ERR_NEEDS_SNAPSHOT: 'RAFT_ERR_NEEDS_SNAPSHOT',
        lib.RAFT_ERR_SNAPSHOT_IN_PROGRESS: 'RAFT_ERR_SNAPSHOT_IN_PROGRESS',
        lib.RAFT_ERR_SNAPSHOT_ALREADY_LOADED: 'RAFT_ERR_SNAPSHOT_ALREADY_LOADED',
        lib.RAFT_ERR_LAST: 'RAFT_ERR_LAST',
    }[err]


class ChangeRaftEntry(object):
    def __init__(self, node_id):
        self.node_id = node_id


class SetRaftEntry(object):
    def __init__(self, key, val):
        self.key = key
        self.val = val


class RaftEntry(object):
    def __init__(self, entry):
        self.term = entry.term
        self.id = entry.id


class Snapshot(object):
    def __init__(self):
        self.members = []


SnapshotMember = collections.namedtuple('SnapshotMember', ['id', 'voting'], verbose=False)


def raft_send_requestvote(raft, udata, node, msg):
    server = ffi.from_handle(udata)
    dst_server = ffi.from_handle(lib.raft_node_get_udata(node))
    server.network.enqueue_msg(msg, server, dst_server)
    return 0


def raft_send_appendentries(raft, udata, node, msg):
    server = ffi.from_handle(udata)
    assert node
    dst_server = ffi.from_handle(lib.raft_node_get_udata(node))
    server.network.enqueue_msg(msg, server, dst_server)

    # Collect statistics
    if server.network.max_entries_in_ae < msg.n_entries:
        server.network.max_entries_in_ae = msg.n_entries

    return 0


def raft_send_snapshot(raft, udata, node):
    return ffi.from_handle(udata).send_snapshot(node)


def raft_applylog(raft, udata, ety, idx):
    try:
        return ffi.from_handle(udata).entry_apply(ety, idx)
    except:
        return lib.RAFT_ERR_SHUTDOWN


def raft_persist_vote(raft, udata, voted_for):
    return ffi.from_handle(udata).persist_vote(voted_for)


def raft_persist_term(raft, udata, term, vote):
    return ffi.from_handle(udata).persist_term(term, vote)


def raft_logentry_offer(raft, udata, ety, ety_idx):
    return ffi.from_handle(udata).entry_append(ety, ety_idx)


def raft_logentry_poll(raft, udata, ety, ety_idx):
    return ffi.from_handle(udata).entry_poll(ety, ety_idx)


def raft_logentry_pop(raft, udata, ety, ety_idx):
    return ffi.from_handle(udata).entry_pop(ety, ety_idx)


def raft_logentry_get_node_id(raft, udata, ety, ety_idx):
    change_entry = ffi.from_handle(ety.data.buf)
    assert isinstance(change_entry, ChangeRaftEntry)
    return change_entry.node_id


def raft_node_has_sufficient_logs(raft, udata, node):
    return ffi.from_handle(udata).node_has_sufficient_entries(node)


def raft_notify_membership_event(raft, udata, node, event_type):
    return ffi.from_handle(udata).notify_membership_event(node, event_type)


def raft_log(raft, node, udata, buf):
    server = ffi.from_handle(lib.raft_get_udata(raft))
    # print(server.id, ffi.string(buf).decode('utf8'))
    if node != ffi.NULL:
        node = ffi.from_handle(lib.raft_node_get_udata(node))
    # if server.id in [1] or (node and node.id in [1]):
    logging.info('{0}>  {1}:{2}: {3}'.format(
        server.network.iteration,
        server.id,
        node.id if node else '',
        ffi.string(buf).decode('utf8'),
    ))


class Message(object):
    def __init__(self, msg, sendor, sendee):
        self.data = msg
        self.sendor = sendor
        self.sendee = sendee


class Network(object):
    def __init__(self, seed=0):
        self.servers = []
        self.messages = []
        self.drop_rate = 0
        self.dupe_rate = 0
        self.partition_rate = 0
        self.iteration = 0
        self.leader = None
        self.ety_id = 0
        self.entries = []
        self.random = random.Random(seed)
        self.partitions = []
        self.no_random_period = False

        self.server_id = 0

        # Information
        self.max_entries_in_ae = 0
        self.leadership_changes = 0
        self.log_pops = 0
        self.num_unique_nodes = 0
        self.num_membership_changes = 0
        self.num_compactions = 0
        self.latest_applied_log_idx = 0

    def add_server(self, server):
        self.server_id += 1
        server.id = self.server_id
        assert server.id not in set([s.id for s in self.servers])
        server.set_network(self)
        self.servers.append(server)

    def push_set_entry(self, k, v):
        for sv in self.active_servers:
            if lib.raft_is_leader(sv.raft):
                ety = ffi.new('msg_entry_t*')
                ety.term = 0
                ety.id = self.new_entry_id()
                ety.type = lib.RAFT_LOGTYPE_NORMAL
                change = ffi.new_handle(SetRaftEntry(k, v))
                ety.data.buf = change
                ety.data.len = ffi.sizeof(ety.data.buf)
                self.entries.append((ety, change))
                e = sv.recv_entry(ety)
                assert e == 0
                break

    def id2server(self, id):
        for server in self.servers:
            if server.id == id:
                return server
            # if lib.raft_get_nodeid(server.raft) == id:
            #     return server
        raise ServerDoesNotExist('Could not find server: {}'.format(id))

    def add_partition(self):
        if len(self.active_servers) <= 1:
            return

        nodes = list(self.active_servers)
        self.random.shuffle(nodes)
        n1 = nodes.pop()
        n2 = nodes.pop()
        self.partitions.append((n1, n2))

    def remove_partition(self):
        self.random.shuffle(self.partitions)
        self.partitions.pop()

    def periodic(self):
        if self.random.randint(1, 100) < self.member_rate:
            if 20 < self.random.randint(1, 100):
                self.add_member()
            else:
                self.remove_member()

        if self.random.randint(1, 100) < self.partition_rate:
            self.add_partition()

        if self.partitions and self.random.randint(1, 100) < self.partition_rate:
            self.remove_partition()

        if self.random.randint(1, 100) < self.client_rate:
            self.push_set_entry(self.random.randint(1, 10), self.random.randint(1, 10))

        for server in self.active_servers:
            if self.no_random_period:
                server.periodic(100)
            else:
                server.periodic(self.random.randint(1, 100))

        # Deadlock detection
        if self.latest_applied_log_idx != 0 and self.latest_applied_log_iteration + 5000 < self.iteration:
            logging.error("deadlock detected iteration:{0} appliedidx:{1}\n".format(
                self.latest_applied_log_iteration,
                self.latest_applied_log_idx,
                ))
            self.diagnotistic_info()
            sys.exit(1)

        # Count leadership changes
        leader_node = lib.raft_get_current_leader_node(self.active_servers[0].raft)
        if leader_node:
            leader = ffi.from_handle(lib.raft_node_get_udata(leader_node))
            if self.leader is not leader:
                self.leadership_changes += 1
            self.leader = leader

    def enqueue_msg(self, msg, sendor, sendee):
        # Drop message if this edge is partitioned
        for partition in self.partitions:
            # Partitions are in one direction
            if partition[0] is sendor and partition[1] is sendee:
                return

        if self.random.randint(1, 100) < self.drop_rate:
            return

        while self.random.randint(1, 100) < self.dupe_rate:
            self._enqueue_msg(msg, sendor, sendee)

        self._enqueue_msg(msg, sendor, sendee)

    def _enqueue_msg(self, msg, sendor, sendee):
        msg_type = ffi.getctype(ffi.typeof(msg))

        msg_size = ffi.sizeof(msg[0])
        new_msg = ffi.cast(ffi.typeof(msg), lib.malloc(msg_size))
        ffi.memmove(new_msg, msg, msg_size)

        if msg_type == 'msg_appendentries_t *':
            size_of_entries = ffi.sizeof(ffi.getctype('msg_entry_t')) * new_msg.n_entries
            new_msg.entries = ffi.cast('msg_entry_t*', lib.malloc(size_of_entries))
            ffi.memmove(new_msg.entries, msg.entries, size_of_entries)

        self.messages.append(Message(new_msg, sendor, sendee))

    def poll_message(self, msg):
        msg_type = ffi.getctype(ffi.typeof(msg.data))

        if msg_type == 'msg_appendentries_t *':
            node = lib.raft_get_node(msg.sendee.raft, msg.sendor.id)
            response = ffi.new('msg_appendentries_response_t*')
            e = lib.raft_recv_appendentries(msg.sendee.raft, node, msg.data, response)
            if lib.RAFT_ERR_SHUTDOWN == e:
                logging.error('Catastrophic')
                print(msg.sendee.debug_log())
                print(msg.sendor.debug_log())
                sys.exit(1)
            elif lib.RAFT_ERR_NEEDS_SNAPSHOT == e:
                pass  # TODO: pretend as if snapshot works
            else:
                self.enqueue_msg(response, msg.sendee, msg.sendor)

        elif msg_type == 'msg_appendentries_response_t *':
            node = lib.raft_get_node(msg.sendee.raft, msg.sendor.id)
            lib.raft_recv_appendentries_response(msg.sendee.raft, node, msg.data)

        elif msg_type == 'msg_requestvote_t *':
            response = ffi.new('msg_requestvote_response_t*')
            node = lib.raft_get_node(msg.sendee.raft, msg.sendor.id)
            lib.raft_recv_requestvote(msg.sendee.raft, node, msg.data, response)
            self.enqueue_msg(response, msg.sendee, msg.sendor)

        elif msg_type == 'msg_requestvote_response_t *':
            node = lib.raft_get_node(msg.sendee.raft, msg.sendor.id)
            e = lib.raft_recv_requestvote_response(msg.sendee.raft, node, msg.data)
            if lib.RAFT_ERR_SHUTDOWN == e:
                msg.sendor.shutdown()

        else:
            assert False

    def poll_messages(self):
        msgs = self.messages

        # Chaos: re-ordered messages
        # self.random.shuffle(msgs)

        self.messages = []
        for msg in msgs:
            self.poll_message(msg)
            for server in self.active_servers:
                if hasattr(server, 'abort_exception'):
                    raise server.abort_exception
                self._check_current_idx_validity(server)
            self._check_election_safety()

    def _check_current_idx_validity(self, server):
        """
        Check that current idx is valid, ie. it exists
        """
        ci = lib.raft_get_current_idx(server.raft)
        if 0 < ci and not lib.raft_get_snapshot_last_idx(server.raft) == ci:
            ety = lib.raft_get_entry_from_idx(server.raft, ci)
            try:
                assert ety
            except Exception:
                print('current idx ', ci)
                print('count', lib.raft_get_log_count(server.raft))
                print('last snapshot', lib.raft_get_snapshot_last_idx(server.raft))
                print(server.debug_log())
                raise

    def _check_election_safety(self):
        """
        FIXME: this is O(n^2)
        Election Safety
        At most one leader can be elected in a given term.
        """

        for i, sv1 in enumerate(net.active_servers):
            if not lib.raft_is_leader(sv1.raft):
                continue
            for sv2 in net.active_servers[i + 1:]:
                term1 = lib.raft_get_current_term(sv1.raft)
                term2 = lib.raft_get_current_term(sv2.raft)
                if lib.raft_is_leader(sv2.raft) and term1 == term2:
                    logging.error("election safety invalidated")
                    print(sv1, sv2, term1)
                    print('partitions:', self.partitions)
                    sys.exit(1)

    def commit_static_configuration(self):
        for server in net.active_servers:
            server.connection_status = NODE_CONNECTED
            for sv in net.active_servers:
                is_self = 1 if sv.id == server.id else 0
                node = lib.raft_add_node(server.raft, sv.udata, sv.id, is_self)

                # FIXME: it's a bit much to expect to set these too
                lib.raft_node_set_voting_committed(node, 1)
                lib.raft_node_set_addition_committed(node, 1)
                lib.raft_node_set_active(node, 1)

    def prep_dynamic_configuration(self):
        """
        Add configuration change for leader's node
        """
        server = self.active_servers[0]
        self.leader = server

        server.set_connection_status(NODE_CONNECTED)
        lib.raft_add_non_voting_node(server.raft, server.udata, server.id, 1)
        lib.raft_become_leader(server.raft)

        # Configuration change entry to bootstrap other nodes
        ety = ffi.new('msg_entry_t*')
        ety.term = 0
        ety.type = lib.RAFT_LOGTYPE_ADD_NODE
        ety.id = self.new_entry_id()
        change = ffi.new_handle(ChangeRaftEntry(server.id))
        ety.data.buf = change
        ety.data.len = ffi.sizeof(ety.data.buf)
        self.entries.append((ety, change))
        e = server.recv_entry(ety)
        assert e == 0

        lib.raft_set_commit_idx(server.raft, 1)
        e = lib.raft_apply_all(server.raft)
        assert e == 0

    def new_entry_id(self):
        self.ety_id += 1
        return self.ety_id

    def remove_server(self, server):
        server.removed = True
        # self.servers = [s for s in self.servers if s is not server]

    @property
    def active_servers(self):
        return [s for s in self.servers if not getattr(s, 'removed', False)]

    def add_member(self):
        if net.num_of_servers <= len(self.active_servers):
            return

        if not self.leader:
            logging.error('no leader')
            return

        leader = self.leader

        if not lib.raft_is_leader(leader.raft):
            return

        if lib.raft_voting_change_is_in_progress(leader.raft):
            # logging.error('{} voting change in progress'.format(server))
            return

        server = RaftServer(self)

        # Create a new configuration entry to be processed by the leader
        ety = ffi.new('msg_entry_t*')
        ety.term = 0
        ety.id = self.new_entry_id()
        ety.type = lib.RAFT_LOGTYPE_ADD_NONVOTING_NODE
        change = ffi.new_handle(ChangeRaftEntry(server.id))
        ety.data.buf = change
        ety.data.len = ffi.sizeof(ety.data.buf)
        assert(lib.raft_entry_is_cfg_change(ety))

        self.entries.append((ety, change))

        e = leader.recv_entry(ety)
        if 0 != e:
            logging.error(err2str(e))
            return
        else:
            self.num_membership_changes += 1

        # Wake up new node
        server.set_connection_status(NODE_CONNECTING)
        assert server.udata
        added_node = lib.raft_add_non_voting_node(server.raft, server.udata, server.id, 1)
        assert added_node

    def remove_member(self):
        if not self.leader:
            logging.error('no leader')
            return

        leader = self.leader
        server = self.random.choice(self.active_servers)

        if not lib.raft_is_leader(leader.raft):
            return

        if lib.raft_voting_change_is_in_progress(leader.raft):
            # logging.error('{} voting change in progress'.format(server))
            return

        if leader == server:
            # logging.error('can not remove leader')
            return

        if server.connection_status in [NODE_CONNECTING, NODE_DISCONNECTING]:
            # logging.error('can not remove server that is changing connection status')
            return

        if NODE_DISCONNECTED == server.connection_status:
            self.remove_server(server)
            return

        # Create a new configuration entry to be processed by the leader
        ety = ffi.new('msg_entry_t*')
        ety.term = 0
        ety.id = self.new_entry_id()
        assert server.connection_status == NODE_CONNECTED
        ety.type = lib.RAFT_LOGTYPE_DEMOTE_NODE
        change = ffi.new_handle(ChangeRaftEntry(server.id))
        ety.data.buf = change
        ety.data.len = ffi.sizeof(ety.data.buf)
        assert(lib.raft_entry_is_cfg_change(ety))

        self.entries.append((ety, change))

        e = leader.recv_entry(ety)
        if 0 != e:
            logging.error(err2str(e))
            return
        else:
            self.num_membership_changes += 1

        # Wake up new node
        assert NODE_CONNECTED == server.connection_status
        server.set_connection_status(NODE_DISCONNECTING)

    def diagnotistic_info(self):
        print()

        info = {
            "Maximum appendentries size": self.max_entries_in_ae,
            "Leadership changes": self.leadership_changes,
            "Log pops": self.log_pops,
            "Unique nodes": self.num_unique_nodes,
            "Membership changes": self.num_membership_changes,
            "Compactions": self.num_compactions,
        }

        for k, v in info.items():
            print(k, v)

        print()

        def abbreviate(k):
            return re.sub(r'([a-z])[a-z]*_', r'\1', k)

        # Servers
        keys = sorted(net.servers[0].debug_statistics().keys())
        print(keys)
        data = [list(map(abbreviate, keys))] + [
            [s.debug_statistics()[key] for key in keys]
            for s in net.servers
            ]
        table = terminaltables.AsciiTable(data)
        print(table.table)


class RaftServer(object):
    def __init__(self, network):

        self.connection_status = NODE_DISCONNECTED

        self.raft = lib.raft_new()
        self.udata = ffi.new_handle(self)

        network.add_server(self)

        self.load_callbacks()
        cbs = ffi.new('raft_cbs_t*')
        cbs.send_requestvote = self.raft_send_requestvote
        cbs.send_appendentries = self.raft_send_appendentries
        cbs.send_snapshot = self.raft_send_snapshot
        cbs.applylog = self.raft_applylog
        cbs.persist_vote = self.raft_persist_vote
        cbs.persist_term = self.raft_persist_term
        cbs.log_offer = self.raft_logentry_offer
        cbs.log_poll = self.raft_logentry_poll
        cbs.log_pop = self.raft_logentry_pop
        cbs.log_get_node_id = self.raft_logentry_get_node_id
        cbs.node_has_sufficient_logs = self.raft_node_has_sufficient_logs
        cbs.notify_membership_event = self.raft_notify_membership_event
        cbs.log = self.raft_log

        lib.raft_set_callbacks(self.raft, cbs, self.udata)

        lib.raft_set_election_timeout(self.raft, 500)

        self.fsm_dict = {}
        self.fsm_log = []

    def __str__(self):
        return '<Server: {0}>'.format(self.id)

    def __repr__(self):
        return 'sv:{0}'.format(self.id)

    def set_connection_status(self, new_status):
        assert(not (self.connection_status == NODE_CONNECTED and new_status == NODE_CONNECTING))
        # logging.warning('{}: {} -> {}'.format(
        #     self,
        #     connectstatus2str(self.connection_status),
        #     connectstatus2str(new_status)))
        self.connection_status = new_status

    def debug_log(self):
        first_idx = lib.raft_get_snapshot_last_idx(self.raft)
        return [(i + first_idx, l.term, l.id) for i, l in enumerate(self.fsm_log)]

    def do_compaction(self):
        # logging.warning('{} snapshotting'.format(self))
        # entries_before = lib.raft_get_log_count(self.raft)

        e = lib.raft_begin_snapshot(self.raft)
        if e != 0:
            return

        assert(lib.raft_snapshot_is_in_progress(self.raft))

        e = lib.raft_end_snapshot(self.raft)
        assert(e == 0)
        if e != 0:
            return

        self.do_membership_snapshot()
        self.snapshot.image = dict(self.fsm_dict)
        self.snapshot.last_term = lib.raft_get_snapshot_last_term(self.raft)
        self.snapshot.last_idx = lib.raft_get_snapshot_last_idx(self.raft)

        self.network.num_compactions += 1

        # logging.warning('{} entries compacted {}'.format(
        #     self,
        #     entries_before - lib.raft_get_log_count(self.raft)
        #     ))

    def periodic(self, msec):
        if self.network.random.randint(1, 100000) < self.network.compaction_rate:
            self.do_compaction()

        e = lib.raft_periodic(self.raft, msec)
        if lib.RAFT_ERR_SHUTDOWN == e:
            self.shutdown()

        # e = lib.raft_apply_all(self.raft)
        # if lib.RAFT_ERR_SHUTDOWN == e:
        #     self.shutdown()
        #     return

        if hasattr(self, 'abort_exception'):
            raise self.abort_exception

    def shutdown(self):
        # logging.error('{} shutting down'.format(self))
        self.set_connection_status(NODE_DISCONNECTED)
        self.network.remove_server(self)

    def set_network(self, network):
        self.network = network

    def load_callbacks(self):
        self.raft_send_requestvote = ffi.callback("int(raft_server_t*, void*, raft_node_t*, msg_requestvote_t*)", raft_send_requestvote)
        self.raft_send_appendentries = ffi.callback("int(raft_server_t*, void*, raft_node_t*, msg_appendentries_t*)", raft_send_appendentries)
        self.raft_send_snapshot = ffi.callback("int(raft_server_t*, void* , raft_node_t*)", raft_send_snapshot)
        self.raft_applylog = ffi.callback("int(raft_server_t*, void*, raft_entry_t*, raft_index_t)", raft_applylog)
        self.raft_persist_vote = ffi.callback("int(raft_server_t*, void*, raft_node_id_t)", raft_persist_vote)
        self.raft_persist_term = ffi.callback("int(raft_server_t*, void*, raft_term_t, raft_node_id_t)", raft_persist_term)
        self.raft_logentry_offer = ffi.callback("int(raft_server_t*, void*, raft_entry_t*, raft_index_t)", raft_logentry_offer)
        self.raft_logentry_poll = ffi.callback("int(raft_server_t*, void*, raft_entry_t*, raft_index_t)", raft_logentry_poll)
        self.raft_logentry_pop = ffi.callback("int(raft_server_t*, void*, raft_entry_t*, raft_index_t)", raft_logentry_pop)
        self.raft_logentry_get_node_id = ffi.callback("int(raft_server_t*, void*, raft_entry_t*, raft_index_t)", raft_logentry_get_node_id)
        self.raft_node_has_sufficient_logs = ffi.callback("int(raft_server_t* raft, void *user_data, raft_node_t* node)", raft_node_has_sufficient_logs)
        self.raft_notify_membership_event = ffi.callback("void(raft_server_t* raft, void *user_data, raft_node_t* node, raft_membership_e)", raft_notify_membership_event)
        self.raft_log = ffi.callback("void(raft_server_t*, raft_node_t*, void*, const char* buf)", raft_log)

    def recv_entry(self, ety):
        # FIXME: leak
        response = ffi.new('msg_entry_response_t*')
        return lib.raft_recv_entry(self.raft, ety, response)

    def get_entry(self, idx):
        idx = idx - lib.raft_get_snapshot_last_idx(self.raft)
        if idx < 0:
            raise IndexError
        try:
            return self.fsm_log[idx]
        except:
            # self.abort_exception = e
            raise

    def _check_log_matching(self, our_log, idx):
        """
        Quality:

        Log Matching: if two logs contain an entry with the same index and
        term, then the logs are identical in all entries up through the given
        index. §5.3

        State Machine Safety: if a server has applied a log entry at a given
        index to its state machine, no other server will ever apply a
        different log entry for the same index. §5.4.3
        """
        for server in self.network.active_servers:
            if server is self:
                continue
            their_commit_idx = lib.raft_get_commit_idx(server.raft)
            if lib.raft_get_commit_idx(self.raft) <= their_commit_idx and idx <= their_commit_idx:
                their_log = lib.raft_get_entry_from_idx(server.raft, idx)

                if their_log == ffi.NULL:
                    assert idx < lib.raft_get_snapshot_last_idx(self.raft)

                if their_log.type in [lib.RAFT_LOGTYPE_NORMAL]:
                    try:
                        assert their_log.term == our_log.term
                        assert their_log.id == our_log.id
                    except Exception as e:
                        ety1 = lib.raft_get_entry_from_idx(self.raft, idx)
                        ety2 = lib.raft_get_entry_from_idx(server.raft, idx)
                        logging.error('ids', ety1.id, ety2.id)
                        logging.error('{0}vs{1} idx:{2} terms:{3} {4} ids:{5} {6}'.format(
                            self, server,
                            idx,
                            our_log.term, their_log.term,
                            our_log.id, their_log.id))
                        self.abort_exception = e

                        logging.error(self.debug_log())
                        logging.error(server.debug_log())
                        return lib.RAFT_ERR_SHUTDOWN

    def entry_apply(self, ety, idx):
        # collect stats
        if self.network.latest_applied_log_idx < idx:
            self.network.latest_applied_log_idx = idx
            self.network.latest_applied_log_iteration = self.network.iteration

        e = self._check_log_matching(ety, idx)
        if e is not None:
            return e

        change = ffi.from_handle(ety.data.buf)

        if ety.type == lib.RAFT_LOGTYPE_NORMAL:
            self.fsm_dict[change.key] = change.val

        elif ety.type == lib.RAFT_LOGTYPE_DEMOTE_NODE:
            if change.node_id == lib.raft_get_nodeid(self.raft):
                # logging.warning("{} shutting down because of demotion".format(self))
                return lib.RAFT_ERR_SHUTDOWN

            # Follow up by removing the node by receiving new entry
            elif lib.raft_is_leader(self.raft):
                new_ety = ffi.new('msg_entry_t*')
                new_ety.term = 0
                new_ety.id = self.network.new_entry_id()
                new_ety.type = lib.RAFT_LOGTYPE_REMOVE_NODE
                new_ety.data.buf = ety.data.buf
                new_ety.data.len = ffi.sizeof(ety.data.buf)
                assert(lib.raft_entry_is_cfg_change(new_ety))
                e = self.recv_entry(new_ety)
                assert e == 0

        elif ety.type == lib.RAFT_LOGTYPE_REMOVE_NODE:
            if change.node_id == lib.raft_get_nodeid(self.raft):
                # logging.warning("{} shutting down because of removal".format(self))
                return lib.RAFT_ERR_SHUTDOWN

        elif ety.type == lib.RAFT_LOGTYPE_ADD_NODE:
            if change.node_id == self.id:
                self.set_connection_status(NODE_CONNECTED)

        elif ety.type == lib.RAFT_LOGTYPE_ADD_NONVOTING_NODE:
            pass

        return 0

    def do_membership_snapshot(self):
        self.snapshot = Snapshot()
        for i in range(0, lib.raft_get_num_nodes(self.raft)):
            n = lib.raft_get_node_from_idx(self.raft, i)
            id = lib.raft_node_get_id(n)
            if 0 == lib.raft_node_is_addition_committed(n):
                id = -1

            self.snapshot.members.append(
                SnapshotMember(id, lib.raft_node_is_voting_committed(n)))

    def load_snapshot(self, snapshot, other):
        # logging.warning('{} loading snapshot'.format(self))
        e = lib.raft_begin_load_snapshot(
            self.raft,
            snapshot.last_term,
            snapshot.last_idx,
        )
        if e == -1:
            return 0
        elif e == lib.RAFT_ERR_SNAPSHOT_ALREADY_LOADED:
            return 0
        elif e == 0:
            pass
        else:
            assert False

        # Send appendentries response for this snapshot
        response = ffi.new('msg_appendentries_response_t*')
        response.success = 1
        response.current_idx = snapshot.last_idx
        response.term = lib.raft_get_current_term(self.raft)
        response.first_idx = response.current_idx
        self.network.enqueue_msg(response, self, other)

        node_id = lib.raft_get_nodeid(self.raft)

        # set membership configuration according to snapshot
        for member in snapshot.members:
            if -1 == member.id:
                continue

            node = lib.raft_get_node(self.raft, member.id)

            if not node:
                udata = ffi.NULL
                try:
                    node_sv = self.network.id2server(member.id)
                    udata = node_sv.udata
                except ServerDoesNotExist:
                    pass

                node = lib.raft_add_node(self.raft, udata, member.id, member.id == node_id)

            lib.raft_node_set_active(node, 1)

            if member.voting and not lib.raft_node_is_voting(node):
                lib.raft_node_set_voting(node, 1)
            elif not member.voting and lib.raft_node_is_voting(node):
                lib.raft_node_set_voting(node, 0)

            if node_id != member.id:
                assert node

        # TODO: this is quite ugly
        # we should have a function that removes all nodes by ourself
        # if (!raft_get_my_node(self->raft)) */
        #     raft_add_non_voting_node(self->raft, NULL, node_id, 1); */

        e = lib.raft_end_load_snapshot(self.raft)
        assert(0 == e)
        assert(lib.raft_get_log_count(self.raft) == 0)

        self.do_membership_snapshot()
        self.snapshot.image = dict(snapshot.image)
        self.snapshot.last_term = snapshot.last_term
        self.snapshot.last_idx = snapshot.last_idx

        assert(lib.raft_get_my_node(self.raft))
        # assert(sv->snapshot_fsm);

        self.fsm_dict = dict(snapshot.image)

        logging.warning('{} loaded snapshot t:{} idx:{}'.format(
            self, snapshot.last_term, snapshot.last_idx))

    def send_snapshot(self, node):
        assert not lib.raft_snapshot_is_in_progress(self.raft)

        # FIXME: Why would this happen?
        if not hasattr(self, 'snapshot'):
            return 0

        node_sv = ffi.from_handle(lib.raft_node_get_udata(node))

        # TODO: Why would this happen?
        # seems odd that we would send something to a node that didn't exist
        if not node_sv:
            return 0

        # NOTE:
        # In a real server we would have to send the snapshot file to the
        # other node. Here we have the convenience of the transfer being
        # "immediate".
        node_sv.load_snapshot(self.snapshot, self)
        return 0

    def persist_vote(self, voted_for):
        # TODO: add disk simulation
        return 0

    def persist_term(self, term, vote):
        # TODO: add disk simulation
        return 0

    def _check_id_monoticity(self, ety):
        """
        Check last entry has smaller ID than new entry.
        This is a virtraft specific check to make sure entry passing is
        working correctly.
        """
        ci = lib.raft_get_current_idx(self.raft)
        if 0 < ci and not lib.raft_get_snapshot_last_idx(self.raft) == ci:
            try:
                prev_ety = lib.raft_get_entry_from_idx(self.raft, ci)
                assert prev_ety
                other_id = prev_ety.id
                assert other_id < ety.id
            except Exception as e:
                logging.error(other_id, ety.id)
                self.abort_exception = e
                raise

    def entry_append(self, ety, ety_idx):
        try:
            assert not self.fsm_log or self.fsm_log[-1].term <= ety.term
        except Exception as e:
            self.abort_exception = e
            # FIXME: consider returning RAFT_ERR_SHUTDOWN
            raise

        self._check_id_monoticity(ety)

        self.fsm_log.append(RaftEntry(ety))

        return 0

    def entry_poll(self, ety, ety_idx):
        self.fsm_log.pop(0)
        return 0

    def _check_committed_entry_popping(self, ety_idx):
        """
        Check we aren't popping a committed entry
        """
        try:
            assert lib.raft_get_commit_idx(self.raft) < ety_idx
        except Exception as e:
            self.abort_exception = e
            return lib.RAFT_ERR_SHUTDOWN
        return 0

    def entry_pop(self, ety, ety_idx):
        # logging.warning("POP {} {}".format(self, ety_idx))

        e = self._check_committed_entry_popping(ety_idx)
        if e != 0:
            return e

        self.fsm_log.pop()
        self.network.log_pops += 1

        change = ffi.from_handle(ety.data.buf)
        if ety.type == lib.RAFT_LOGTYPE_DEMOTE_NODE:
            pass

        elif ety.type == lib.RAFT_LOGTYPE_REMOVE_NODE:
            if change.node_id == lib.raft_get_nodeid(self.raft):
                self.set_connection_status(NODE_CONNECTED)

        elif ety.type == lib.RAFT_LOGTYPE_ADD_NONVOTING_NODE:
            if change.node_id == lib.raft_get_nodeid(self.raft):
                logging.error("POP disconnect {} {}".format(self, ety_idx))
                self.set_connection_status(NODE_DISCONNECTED)

        elif ety.type == lib.RAFT_LOGTYPE_ADD_NODE:
            if change.node_id == lib.raft_get_nodeid(self.raft):
                self.set_connection_status(NODE_CONNECTING)

        return 0

    def node_has_sufficient_entries(self, node):
        assert(not lib.raft_node_is_voting(node))

        ety = ffi.new('msg_entry_t*')
        ety.term = 0
        ety.id = self.network.new_entry_id()
        ety.type = lib.RAFT_LOGTYPE_ADD_NODE
        change = ffi.new_handle(ChangeRaftEntry(lib.raft_node_get_id(node)))
        ety.data.buf = change
        ety.data.len = ffi.sizeof(ety.data.buf)
        self.network.entries.append((ety, change))
        assert(lib.raft_entry_is_cfg_change(ety))
        # FIXME: leak
        e = self.recv_entry(ety)
        # print(err2str(e))
        assert e == 0
        return 0

    def notify_membership_event(self, node, event_type):
        # Convenience: Ensure that added node has udata set
        if event_type == lib.RAFT_MEMBERSHIP_ADD:
            node_id = lib.raft_node_get_id(node)
            try:
                server = self.network.id2server(node_id)
            except ServerDoesNotExist:
                pass
            else:
                node = lib.raft_get_node(self.raft, node_id)
                lib.raft_node_set_udata(node, server.udata)

    def debug_statistics(self):
        return {
            "node": lib.raft_get_nodeid(self.raft),
            "state": state2str(lib.raft_get_state(self.raft)),
            "current_idx": lib.raft_get_current_idx(self.raft),
            "last_log_term": lib.raft_get_last_log_term(self.raft),
            "current_term": lib.raft_get_current_term(self.raft),
            "commit_idx": lib.raft_get_commit_idx(self.raft),
            "last_applied_idx": lib.raft_get_last_applied_idx(self.raft),
            "log_count": lib.raft_get_log_count(self.raft),
            "peers": lib.raft_get_num_nodes(self.raft),
            "voting_peers": lib.raft_get_num_voting_nodes(self.raft),
            "connection_status": connectstatus2str(self.connection_status),
            "voting_change_in_progress": lib.raft_voting_change_is_in_progress(self.raft),
            "removed": getattr(self, 'removed', False),
        }


if __name__ == '__main__':
    try:
        args = docopt.docopt(__doc__, version='virtraft 0.1')
    except docopt.DocoptExit as e:
        print(e)
        sys.exit()

    if args['--verbose'] or args['--log_level']:
        coloredlogs.install(fmt='%(asctime)s %(levelname)s %(message)s')
        level = logging.DEBUG
        if args['--log_level']:
            level = int(args['--log_level'])
        logging.basicConfig(level=level, format='%(asctime)s %(message)s')

    net = Network(int(args['--seed']))

    net.dupe_rate = int(args['--dupe_rate'])
    net.drop_rate = int(args['--drop_rate'])
    net.client_rate = int(args['--client_rate'])
    net.member_rate = int(args['--member_rate'])
    net.compaction_rate = int(args['--compaction_rate'])
    net.partition_rate = int(args['--partition_rate'])
    net.no_random_period = 1 == int(args['--no_random_period'])

    net.num_of_servers = int(args['--servers'])

    if net.member_rate == 0:
        for i in range(0, int(args['--servers'])):
            RaftServer(net)
        net.commit_static_configuration()
    else:
        RaftServer(net)
        net.prep_dynamic_configuration()

    for i in range(0, int(args['--iterations'])):
        net.iteration += 1
        try:
            net.periodic()
            net.poll_messages()
        except:
            # for server in net.servers:
            #     print(server, [l.term for l in server.fsm_log])
            raise

    net.diagnotistic_info()
