#pragma once

#include "ns3/core-module.h"
#include "ns3/mobility-module.h"
#include "ns3/network-module.h"

#include <cmath>
#include <map>
#include <vector>

namespace wsn {

struct NodeState
{
  uint32_t id;
  double energy;
  double trust;
  bool alive;
};

class WsnNetwork
{
public:
  WsnNetwork(uint32_t nNodes, double area, double commRange, double initEnergy, uint32_t seed)
    : m_nNodes(nNodes), m_area(area), m_commRange(commRange), m_initEnergy(initEnergy), m_seed(seed)
  {
  }

  void Initialize()
  {
    ns3::RngSeedManager::SetSeed(m_seed);
    ns3::RngSeedManager::SetRun(1);

    m_nodes.Create(m_nNodes);
    ns3::MobilityHelper mobility;
    mobility.SetPositionAllocator("ns3::RandomRectanglePositionAllocator",
                                  "X",
                                  ns3::StringValue("ns3::UniformRandomVariable[Min=0.0|Max=" + std::to_string(m_area) + "]"),
                                  "Y",
                                  ns3::StringValue("ns3::UniformRandomVariable[Min=0.0|Max=" + std::to_string(m_area) + "]"));
    mobility.SetMobilityModel("ns3::ConstantPositionMobilityModel");
    mobility.Install(m_nodes);

    auto sink = m_nodes.Get(0)->GetObject<ns3::MobilityModel>();
    sink->SetPosition(ns3::Vector(m_area / 2.0, m_area / 2.0, 0.0));

    m_state.clear();
    for (uint32_t i = 0; i < m_nNodes; ++i)
    {
      m_state.push_back({i, m_initEnergy, 1.0, true});
    }

    BuildAdjacency();
  }

  void BuildAdjacency()
  {
    m_adj.clear();
    for (uint32_t i = 0; i < m_nNodes; ++i)
    {
      std::vector<uint32_t> neigh;
      auto p1 = m_nodes.Get(i)->GetObject<ns3::MobilityModel>()->GetPosition();
      for (uint32_t j = 0; j < m_nNodes; ++j)
      {
        if (i == j)
          continue;
        auto p2 = m_nodes.Get(j)->GetObject<ns3::MobilityModel>()->GetPosition();
        double d = std::hypot(p1.x - p2.x, p1.y - p2.y);
        if (d <= m_commRange)
          neigh.push_back(j);
      }
      m_adj[i] = neigh;
    }
  }

  double Distance(uint32_t a, uint32_t b) const
  {
    auto p1 = m_nodes.Get(a)->GetObject<ns3::MobilityModel>()->GetPosition();
    auto p2 = m_nodes.Get(b)->GetObject<ns3::MobilityModel>()->GetPosition();
    return std::hypot(p1.x - p2.x, p1.y - p2.y);
  }

  double ProgressToSink(uint32_t cur, uint32_t nxt) const
  {
    return Distance(cur, 0) - Distance(nxt, 0);
  }

  ns3::NodeContainer& Nodes() { return m_nodes; }
  std::vector<NodeState>& State() { return m_state; }
  const std::vector<NodeState>& State() const { return m_state; }
  const std::map<uint32_t, std::vector<uint32_t>>& Adj() const { return m_adj; }

private:
  uint32_t m_nNodes;
  double m_area;
  double m_commRange;
  double m_initEnergy;
  uint32_t m_seed;

  ns3::NodeContainer m_nodes;
  std::vector<NodeState> m_state;
  std::map<uint32_t, std::vector<uint32_t>> m_adj;
};

} // namespace wsn
