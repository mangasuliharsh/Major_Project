#include "ns3/core-module.h"
#include "ns3/network-module.h"
#include "ns3/netanim-module.h"

#include "../ml_model/secure_ml_policy.h"
#include "../network/wsn_network.h"

#include <algorithm>
#include <fstream>
#include <map>
#include <random>
#include <set>
#include <string>
#include <vector>

using namespace ns3;

NS_LOG_COMPONENT_DEFINE("SecureMlWsnMain");

namespace {

enum class ProtocolType
{
  AODV_LIKE,
  LEACH_LIKE,
  SECURE_ML
};

struct Metrics
{
  uint64_t generated{0};
  uint64_t delivered{0};
  uint64_t controlPackets{0};
  double totalDelay{0.0};
  double totalEnergySpent{0.0};
  int32_t fndRound{-1};
};

void BuildPegasisChain(const wsn::WsnNetwork& net, std::map<uint32_t, uint32_t>& chain)
{
  chain.clear();
  const uint32_t nNodes = net.Nodes().GetN();
  std::set<uint32_t> unvisited;
  for (uint32_t i = 1; i < nNodes; ++i)
    unvisited.insert(i);

  if (unvisited.empty())
    return;

  uint32_t current = *unvisited.begin();
  while (!unvisited.empty())
  {
    unvisited.erase(current);
    if (unvisited.empty())
    {
      chain[current] = 0;
      break;
    }

    uint32_t best = *unvisited.begin();
    double bestDist = net.Distance(current, best);
    for (auto c : unvisited)
    {
      double d = net.Distance(current, c);
      if (d < bestDist)
      {
        bestDist = d;
        best = c;
      }
    }
    chain[current] = best;
    current = best;
  }
}

bool AnySensorDead(const std::vector<wsn::NodeState>& state)
{
  for (size_t i = 1; i < state.size(); ++i)
  {
    if (!state[i].alive)
      return true;
  }
  return false;
}

void ApplyEnergy(std::vector<wsn::NodeState>& state, uint32_t src, uint32_t dst, Metrics& metrics)
{
  const double tx = 0.004;
  const double rx = 0.002;
  state[src].energy -= tx;
  state[dst].energy -= rx;
  metrics.totalEnergySpent += tx + rx;
  if (state[src].energy <= 0)
    state[src].alive = false;
  if (state[dst].energy <= 0)
    state[dst].alive = false;
}

std::string ProtocolName(ProtocolType p)
{
  switch (p)
  {
  case ProtocolType::AODV_LIKE:
    return "aodv_like";
  case ProtocolType::LEACH_LIKE:
    return "leach_like";
  case ProtocolType::SECURE_ML:
    return "secure_ml";
  }
  return "unknown";
}

} // namespace

int main(int argc, char* argv[])
{
  std::string protocol = "secure_ml";
  uint32_t nodes = 200;
  uint32_t rounds = 1000;
  uint32_t packetsPerRound = 20;
  double area = 500.0;
  double commRange = 120.0;
  double initEnergy = 2.0;
  double attackFraction = 0.1;
  uint32_t seed = 7;
  std::string csvOut = "ns3/csv_logs/wsn_results.csv";
  std::string animOut = "ns3/netanim/secure_ml_anim.xml";

  CommandLine cmd;
  cmd.AddValue("protocol", "aodv_like|leach_like|pegasis_like|secure_ml", protocol);
  cmd.AddValue("nodes", "Number of nodes", nodes);
  cmd.AddValue("rounds", "Simulation rounds", rounds);
  cmd.AddValue("packetsPerRound", "Packets generated each round", packetsPerRound);
  cmd.AddValue("area", "Square area side length", area);
  cmd.AddValue("commRange", "Communication range", commRange);
  cmd.AddValue("initEnergy", "Initial energy (J)", initEnergy);
  cmd.AddValue("attackFraction", "Fraction malicious", attackFraction);
  cmd.AddValue("seed", "Random seed", seed);
  cmd.AddValue("csvOut", "CSV output file", csvOut);
  cmd.AddValue("animOut", "NetAnim XML output file", animOut);
  cmd.Parse(argc, argv);

  ProtocolType p = ProtocolType::SECURE_ML;
  if (protocol == "aodv_like")
    p = ProtocolType::AODV_LIKE;
  else if (protocol == "leach_like")
    p = ProtocolType::LEACH_LIKE;

  wsn::WsnNetwork net(nodes, area, commRange, initEnergy, seed);
  net.Initialize();

  AnimationInterface anim(animOut);
  anim.SetMobilityPollInterval(Seconds(1.0));

  // node descriptions/colors for visual debugging
  for (uint32_t i = 0; i < nodes; ++i)
  {
    anim.UpdateNodeDescription(i, i == 0 ? "Sink" : ("N" + std::to_string(i)));
    if (i == 0)
      anim.UpdateNodeColor(i, 255, 0, 0);
    else
      anim.UpdateNodeColor(i, 0, 0, 255);
  }

  wsn::SecureMlPolicy policy;
  policy.InitializeAttacks(nodes, attackFraction, seed);

  std::map<uint32_t, uint32_t> chain;
  BuildPegasisChain(net, chain);

  Metrics metrics;
  auto& state = net.State();
  const auto& adj = net.Adj();

  for (uint32_t round = 0; round < rounds; ++round)
  {
    std::vector<uint32_t> sources;
    for (uint32_t i = 1; i < nodes; ++i)
    {
      if (state[i].alive)
        sources.push_back(i);
    }

    std::mt19937 gen(seed + round * 73);
    std::shuffle(sources.begin(), sources.end(), gen);
    if (sources.size() > packetsPerRound)
      sources.resize(packetsPerRound);

    for (auto src : sources)
    {
      metrics.generated++;
      uint32_t current = src;
      uint32_t ttl = 15;
      uint32_t hops = 0;

      while (ttl > 0 && current != 0 && state[current].alive)
      {
        ttl--;
        hops++;

        uint32_t next = UINT32_MAX;
        if (protocol == "pegasis_like")
        {
          auto it = chain.find(current);
          next = (it == chain.end() ? UINT32_MAX : it->second);
        }
        else if (p == ProtocolType::AODV_LIKE)
        {
          next = policy.NextHopAodvLike(net, current);
          metrics.controlPackets++;
        }
        else if (p == ProtocolType::LEACH_LIKE)
        {
          next = policy.NextHopLeachLike(net, current);
          metrics.controlPackets++;
        }
        else
        {
          next = policy.NextHopSecureMl(net, current);
        }

        if (next == UINT32_MAX)
          break;

        if (policy.ShouldSelectiveDrop(next, seed, round, hops))
        {
          ApplyEnergy(state, current, next, metrics);
          policy.UpdateTrust(state, next, false);
          break;
        }

        ApplyEnergy(state, current, next, metrics);
        policy.UpdateTrust(state, next, true);

        if (p == ProtocolType::SECURE_ML)
        {
          double reward = 1.0 + 0.5 * state[next].energy + 0.8 * state[next].trust;
          if (next == 0)
            reward += 2.0;
          policy.QUpdate(current, next, next, adj, reward);
        }

        if (next != 0)
        {
          uint8_t energyShade = static_cast<uint8_t>(std::max(0.0, std::min(255.0, state[next].energy / initEnergy * 255.0)));
          anim.UpdateNodeColor(next, 0, energyShade, 255 - energyShade);
        }

        current = next;
      }

      if (current == 0)
      {
        metrics.delivered++;
        metrics.totalDelay += hops;
      }
    }

    if (metrics.fndRound < 0 && AnySensorDead(state))
      metrics.fndRound = static_cast<int32_t>(round);
  }

  std::ofstream out(csvOut, std::ios::app);
  out << ProtocolName(p) << ',' << nodes << ',' << rounds << ',' << metrics.generated << ',' << metrics.delivered << ','
      << (metrics.generated ? static_cast<double>(metrics.delivered) / metrics.generated : 0.0) << ','
      << (metrics.delivered ? metrics.totalDelay / metrics.delivered : 0.0) << ','
      << (metrics.delivered ? metrics.totalEnergySpent / metrics.delivered : 0.0) << ','
      << (metrics.delivered ? static_cast<double>(metrics.controlPackets) / metrics.delivered : 0.0) << ','
      << metrics.fndRound << '\n';
  out.close();

  NS_LOG_UNCOND("Protocol=" << ProtocolName(p) << " completed. CSV=" << csvOut << " NetAnim=" << animOut);

  Simulator::Stop(Seconds(0.0));
  Simulator::Run();
  Simulator::Destroy();
  return 0;
}
