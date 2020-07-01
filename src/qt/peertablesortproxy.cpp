// Copyright (c) 2020 The Bitcoin Core developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include <qt/peertablesortproxy.h>

#include <qt/peertablemodel.h>

#include <QModelIndex>
#include <QString>
#include <QVariant>

PeerTableSortProxy::PeerTableSortProxy(QObject* parent)
    : QSortFilterProxyModel(parent)
{
}

bool PeerTableSortProxy::lessThan(const QModelIndex& left_index, const QModelIndex& right_index) const
{
    const CNodeStats left_stats = sourceModel()->data(left_index, PeerTableModel::StatsRole).value<CNodeCombinedStats*>()->nodeStats;
    const CNodeStats right_stats = sourceModel()->data(right_index, PeerTableModel::StatsRole).value<CNodeCombinedStats*>()->nodeStats;

    switch (static_cast<PeerTableModel::ColumnIndex>(left_index.column())) {
    case PeerTableModel::NetNodeId:
        return left_stats.nodeid < right_stats.nodeid;
    case PeerTableModel::Address:
        return left_stats.addrName.compare(right_stats.addrName) < 0;
    case PeerTableModel::Network:
        return left_stats.m_network < right_stats.m_network;
    case PeerTableModel::Ping:
        return left_stats.m_min_ping_usec < right_stats.m_min_ping_usec;
    case PeerTableModel::Sent:
        return left_stats.nSendBytes < right_stats.nSendBytes;
    case PeerTableModel::Received:
        return left_stats.nRecvBytes < right_stats.nRecvBytes;
    case PeerTableModel::Subversion:
        return left_stats.cleanSubVer.compare(right_stats.cleanSubVer) < 0;
    } // no default case, so the compiler can warn about missing cases
    assert(false);
}
