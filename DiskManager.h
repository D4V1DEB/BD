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
    char diskPath[512];       // Ruta del disco
    char esquemaPath[512];    // Ruta esquema.txt
    long long espacioTotal;

    int sectoresTotal() const;
    void getsectorPath(int, int, int, int, char*) const; // Escribir ruta en un buffer

public:
    DiskManager(const char* path);
    void loadConfig();
    void info();
    void diskEstructura();
    bool writeRegistro(const char*);
    void leerRegistro(int, int, int, int, char*) const;
    const char* tipoDato(const char*) const;
    void esquemaFile(const char*, const char*, const char*);
    void cargarCSV(const char*);
    void mostrarSector(int, int, int, int) const;
};

#endif
