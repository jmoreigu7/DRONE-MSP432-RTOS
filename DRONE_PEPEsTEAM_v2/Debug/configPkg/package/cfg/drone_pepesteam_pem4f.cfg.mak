# invoke SourceDir generated makefile for drone_pepesteam.pem4f
drone_pepesteam.pem4f: .libraries,drone_pepesteam.pem4f
.libraries,drone_pepesteam.pem4f: package/cfg/drone_pepesteam_pem4f.xdl
	$(MAKE) -f C:\Users\jeffr\workspace_v8\DRONE_PEPEsTEAM_v2/src/makefile.libs

clean::
	$(MAKE) -f C:\Users\jeffr\workspace_v8\DRONE_PEPEsTEAM_v2/src/makefile.libs clean

