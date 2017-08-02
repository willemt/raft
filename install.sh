sudo mkdir -p /usr/local/include/raft
sudo cp $(dirname $0)/include/raft.h /usr/local/include/raft/
sudo cp $(dirname $0)/libcraft.* /usr/local/lib
sudo ldconfig

