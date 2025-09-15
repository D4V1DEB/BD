#ifndef DISKMANAGER_H
#define DISKMANAGER_H

#include <cstring>
#include <cstdio>

class DiskManager {
private:
    int numplatos;
    int numsuperficies;
    int numpistas;
    int numsectores;
    int bytesxSec;
    int bytesxBloq;
    // NOLINTNEXTLINE(cpp:S5945)
    char diskPath[512]{};       // Ruta del disco
    char esquemaPath[512]{"esquema.txt"};     // Ruta esquema.txt
    long long espacioTotal{0};
    char sectorBuffer[512];
    int sectorSize;

    int sectoresTotal() const;
    void getsectorPath(int, int, int, int, char*) const; // Escribir ruta en un buffer

public:
    explicit DiskManager(const char* path);
    void loadConfig();
    void info();
    void diskEstructura();
    bool writeRegistro(const char*);
    bool flushSectorBuffer();
    bool tryWriteSector(int, int, int, int);
    bool appendToSectorBuffer(const char*, int);
    void leerRegistro(int, int, int, int, char*) const;
    const char* tipoDato(const char*) const;
    void esquemaFile(const char*, const char*, const char*);
    void cargarCSV(const char*);
    void mostrarSector(int, int, int, int) const;
};

#endif
