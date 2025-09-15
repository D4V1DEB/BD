#ifndef DISKMANAGER_H
#define DISKMANAGER_H

#include <cstring>
#include <cstdio>
#include <fstream>
#include <iostream>

class DiskManager {
private:
    int numplatos;
    int numsuperficies;
    int numpistas;
    int numsectores;
    int bytesxSec;
    int bytesxBloq;
    char diskPath[512]{};       
    char esquemaPath[512]{"esquema.txt"};     
    long long espacioTotal{0};
    char sectorBuffer[512]{'\0'};
    int sectorSize{0};

    int sectoresTotal() const;
    void getsectorPath(int, int, int, int, char*) const; 

public:
    explicit DiskManager(const char* path);
    void loadConfig();
    void info();
    void diskEstructura();
    bool writeRegistro(const char*);
    bool flushSectorBuffer();
    bool tryWriteSector(int, int, int, int);
    bool addNewlineIfNeeded();
    bool appendToSectorBuffer(const char*, int);
    void leerRegistro(int, int, int, int, char*) const;
    const char* tipoDato(const char*) const;
    void esquemaFile(const char*, const char*, const char*) const;
    void cargarCSV(const char*);
    void mostrarSector(int, int, int, int) const;
    
private:
    static const int MAX_CAMPOS = 100;
    static const int MAX_CAMPO = 128;
    
    bool flushSectorBufferByPlato(int plato);
    bool flushSectorBufferBySuperficie(int plato, int superficie);
    bool flushSectorBufferByPista(int plato, int superficie, int pista);
    
    bool tablaExisteEnEsquema(const char* tableName) const;
    void escribirEsquema(const char* tableName, const char* headers, const char* types) const;
    void procesarCamposEsquema(std::ofstream& esquemaOut, const char* headers, const char* types) const;
    void procesarCampoIndividual(std::ofstream& esquemaOut, const char* headers, const char* types, int& i, int& j, bool& inHeader) const;
    
    void dividirCamposCSV(const char* linea, char campos[][MAX_CAMPO], int& cantidad) const;
    bool leerYProcesarEncabezados(std::ifstream& csvFile, char encabezados[][MAX_CAMPO], char tipos[][MAX_CAMPO], int& numCampos) const;
    void inferirTiposDatos(std::ifstream& csvFile, char tipos[][MAX_CAMPO], int numCampos) const;
    void procesarLineaParaInferencia(const char* lineaBuffer, char tipos[][MAX_CAMPO], int numCampos, char valores[][MAX_CAMPO]) const;
    void actualizarTipoSiEsNecesario(char* tipoActual, const char* valor) const;
    void procesarRegistrosCSV(std::ifstream& csvFile, [[maybe_unused]] int numCampos);
    void procesarLineaCSV(const char* lineaBuffer);
    void guardarEsquema(const char* csvFilePath, char encabezados[][MAX_CAMPO], char tipos[][MAX_CAMPO], int numCampos);
};

#endif