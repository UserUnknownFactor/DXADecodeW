TARGETD		:= DXADecode.exe
TARGETE		:= DXAEncode.exe

CC		:= g++
LD		:= g++
CFLAGS		:= -Wall
LDFLAGS		:= -Wall

.PHONY: clean

all: $(TARGETE) $(TARGETD)

$(TARGETE): DXArchiveEncode.o DXArchive.o FileLib.o
	$(LD) -o $(TARGETE) $^ $(LDFLAGS)
	
$(TARGETD): DXArchiveDecode.o DXArchive.o DXArchiveVer5.o FileLib.o
	$(LD) -o $(TARGETD) $^ $(LDFLAGS)

DXArchiveDecode.o: DXArchiveDecode.cpp DXArchive.h DXArchiveVer5.h
	$(CC) -c $< -o $@ $(CFLAGS)

DXArchiveEncode.o: DXArchiveEncode.cpp DXArchive.h DXArchiveVer5.h FileLib.h DataType.h
	$(CC) -c $< -o $@ $(CFLAGS)

FileLib.o: FileLib.cpp FileLib.h DataType.h
	$(CC) -c $< -o $@ $(CFLAGS)
	
DXArchiveVer5.o: DXArchiveVer5.cpp DXArchiveVer5.h
	$(CC) -c $< -o $@ $(CFLAGS)

DXArchive.o: DXArchive.cpp DXArchive.h
	$(CC) -c $< -o $@ $(CFLAGS)

clean:
	rm -f *.o
