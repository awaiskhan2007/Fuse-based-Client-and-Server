all:
	g++ -Wall nfs-server.c nfs-object.pb.cc -lcci -lprotobuf `pkg-config sqlite3 --cflags --libs` -o nfs-server
