#include "ns3/core-module.h"
#include "ns3/mobility-module.h"
#include "ns3/network-module.h"
#include "ns3/wifi-module.h"
#include "ns3/random-variable.h"

#include <algorithm>
#include <vector>

using namespace ns3;
using std::size_t;


class FileSMS
{
public:
    FileSMS(unsigned int id, size_t size)
        : m_id(id)
        , m_fileSize(size)
    {
    }

    unsigned int GetFileId() const
    {
        return m_id;
    }

    size_t GetFileSize() const
    {
        return m_fileSize;
    }

    bool operator==(const FileSMS &other) const
    {
        return m_id == other.m_id;
    }

    bool operator!=(const FileSMS &other) const
    {
        return !(*this == other);
    }

private:
    unsigned int m_id;
    size_t m_fileSize;
};


/**
 * Returns the total number of mobile nodes running in the simulation
 */
unsigned int getNumberOfMobileNodes()
{
    return 30; // This is just an example, the number may be different
}

/**
 * Returns the duration of the simulation, i.e. the amount of seconds
 * after which the simulation must be stopped
 */
double getSimulationDuration()
{
    return 900.0; // This is just an example, the number may be different
}

/**
 * Installs the mobility component on all the nodes in NodeContainer c
 */
void installMobility(NodeContainer &c)
{
    MobilityHelper mobility;

    // These are just examples, the parameters may be different
    mobility.SetPositionAllocator("ns3::GridPositionAllocator",
                                  "MinX", DoubleValue(0.0),
                                  "MinY", DoubleValue(0.0),
                                  "DeltaX", DoubleValue(5.0),
                                  "DeltaY", DoubleValue(10.0),
                                  "GridWidth", UintegerValue(5),
                                  "LayoutType", StringValue("RowFirst"));
    mobility.SetMobilityModel("ns3::RandomWalk2dMobilityModel",
                              "Bounds", RectangleValue(Rectangle(-100, 100, -100, 100)));

    mobility.Install(c);
}

/**
 * Install a WiFiNetDevice on each node in NodeContainer c and sets the WiFi parameters.
 * The resulting NetDevices will be stored inside the NetDeviceContainer passed as parameter
 * (the function will overwrite the NetDeviceContainer).
 */
void installWifi(NodeContainer &c, NetDeviceContainer &devices)
{
    // Modulation and wifi channel bit rate
    std::string phyMode("OfdmRate24Mbps");

    // Fix non-unicast data rate to be the same as that of unicast
    Config::SetDefault("ns3::WifiRemoteStationManager::NonUnicastMode", StringValue(phyMode));

    WifiHelper wifi;
    wifi.SetStandard(WIFI_PHY_STANDARD_80211a);

    YansWifiPhyHelper wifiPhy = YansWifiPhyHelper::Default();
    wifiPhy.SetPcapDataLinkType(YansWifiPhyHelper::DLT_IEEE802_11_RADIO);

    YansWifiChannelHelper wifiChannel;
    wifiChannel.SetPropagationDelay("ns3::ConstantSpeedPropagationDelayModel");
    wifiChannel.AddPropagationLoss("ns3::RangePropagationLossModel",
                                   "MaxRange", DoubleValue(25.0));
    wifiChannel.AddPropagationLoss("ns3::NakagamiPropagationLossModel",
                                   "m0", DoubleValue(1.0),
                                   "m1", DoubleValue(1.0),
                                   "m2", DoubleValue(1.0));
    wifiPhy.SetChannel(wifiChannel.Create());

    // Add a non-QoS upper mac
    NqosWifiMacHelper wifiMac = NqosWifiMacHelper::Default();
    // Set it to adhoc mode
    wifiMac.SetType("ns3::AdhocWifiMac");

    // Disable rate control
    wifi.SetRemoteStationManager("ns3::ConstantRateWifiManager",
                                 "DataMode", StringValue(phyMode),
                                 "ControlMode", StringValue(phyMode));

    devices = wifi.Install(wifiPhy, wifiMac, c);
}

/**
 * This function returns the files that are available in a mobile node at the beginning of the simulation.
 * The student has to call getInitialFileList exactly once for each mobile node in the simulation.
 * Each FileSMS object has an id and a size in KB. Different files may have different sizes.
 * The popularity of different files may differ. The popularity of a file is unknown at the beginning of the simulation.
 */
std::vector<FileSMS> getInitialFileList()
{
    // This is just an example, the implementation used in the evaluation may differ in:
    //   - The value of totalFileCount (size of the entire catalog)
    //   - File popularity distribution
    //   - Max number of files per node
    //   - File size (keep in mind that different files may have different sizes)
    //   - Something else
    static const unsigned int totalFileCount = 100u;
    static const unsigned int maxFileCountPerNode = 10u;
    static const unsigned int minFileSize = 1024u; // 1 KiB
    static const unsigned int maxFileSize = 20971520u; // 20 MiB

    static std::vector<FileSMS> allFiles;
    if (allFiles.empty()) {
        // create all files, with a randomly generated size
        allFiles.reserve(totalFileCount);
        UniformVariable fileSizeRand;
        for (unsigned int i = 1; i <= totalFileCount; i++) {
            allFiles.push_back(FileSMS(i, fileSizeRand.GetInteger(minFileSize, maxFileSize)));
        }
    }

    // select how many files the node initially has
    static UniformVariable numOfFilesRand;
    unsigned int numOfFiles = numOfFilesRand.GetInteger(0, maxFileCountPerNode);
    std::vector<FileSMS> files;
    files.reserve(numOfFiles);

    // assign files to the node
    static ZipfVariable zipfRandom(totalFileCount, 1.1);
    for (unsigned int i = 0; i < numOfFiles; i++) {
        FileSMS f = allFiles.at(zipfRandom.GetInteger() - 1);
        std::vector<FileSMS>::const_iterator it = std::find(files.begin(), files.end(), f);
        while (it != files.end()) {
            f = allFiles.at(zipfRandom.GetInteger() - 1);
            it = std::find(files.begin(), files.end(), f);
        }
        files.push_back(f);
    }

    return files;
}
