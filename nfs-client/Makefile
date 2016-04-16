all:
	g++ -Wall nfs-client.c nfs-object.pb.cc  -lcci -lprotobuf `pkg-config fuse3  sqlite3 --cflags --libs` -o nfs-client
