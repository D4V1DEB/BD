#include "DiskManager.h"
#include <iostream>
#include <fstream>
#include <filesystem>

using namespace std;
namespace fs = std::filesystem;

DiskManager::DiskManager(const char* path) {
    strncpy(diskPath, path, sizeof(diskPath) - 1);
    diskPath[sizeof(diskPath) - 1] = '\0'; 
    strcpy(esquemaPath, "esquema.txt");

    if (fs::exists(diskPath)) {
        loadConfig();
    } else {
        diskEstructura();
    }
}

void DiskManager::loadConfig() {
    const char* configPath = "config/disk.txt";
    ifstream config(configPath);
    if (!config.is_open()) {
        cerr << "Error: No se pudo abrir el archivo de configuracion." << endl;
        return;
    }

    config >> numplatos >> numsuperficies >> numpistas >> numsectores >> bytesxSec >> bytesxBloq;
    config.close();

    espacioTotal = static_cast<long long>(numplatos) * numsuperficies * numpistas * numsectores * bytesxSec;
    cout << "[DEBUG] config cargada exitosamenten" << endl;
}

int DiskManager::sectoresTotal() const {
    return numplatos * numsuperficies * numpistas * numsectores;
}

void DiskManager::getsectorPath(int plato, int superficie, int pista, int sector, char* buffer) const {
    snprintf(buffer, 600, "%s/Plato%d/Superficie%d/Pista%d/Sector%d.txt", diskPath, plato, superficie, pista, sector);
}

void DiskManager::diskEstructura() {
    for (int p = 0; p < numplatos; ++p) {
        for (int s = 0; s < numsuperficies; ++s) {
            for (int t = 0; t < numpistas; ++t) {
                char path[600];
                snprintf(path, sizeof(path), "%s/Plato%d/Superficie%d/Pista%d", diskPath, p, s, t);
                fs::create_directories(path);
            }
        }
    }
}


bool DiskManager::writeRegistro(const char* registro) {
    auto registroLength = static_cast<int>(strlen(registro));

    if (int espacioNecesario = registroLength + (sectorSize > 0 ? 1 : 0); 
        sectorSize + espacioNecesario > 512) {
        return flushSectorBuffer() && appendToSectorBuffer(registro, registroLength);
    }
    
    return appendToSectorBuffer(registro, registroLength);
}

bool DiskManager::addNewlineIfNeeded() {
    if (sectorSize + 1 >= 512) {
        return flushSectorBuffer();
    }
    
    strcat(sectorBuffer, "\n");
    sectorSize += 1;
    return true;
}

bool DiskManager::flushSectorBuffer() {
    for (int p = 0; p < numplatos; ++p) {
        if (flushSectorBufferByPlato(p)) {
            return true;
        }
    }
    
    cerr << "Error: No hay sectores disponibles." << endl;
    return false;
}
bool DiskManager::flushSectorBufferByPlato(int plato) {
    for (int s = 0; s < numsuperficies; ++s) {
        if (flushSectorBufferBySuperficie(plato, s)) {
            return true;
        }
    }
    return false;
}


bool DiskManager::flushSectorBufferBySuperficie(int plato, int superficie) {
    for (int t = 0; t < numpistas; ++t) {
        if (flushSectorBufferByPista(plato, superficie, t)) {
            return true;
        }
    }
    return false;
}

bool DiskManager::flushSectorBufferByPista(int plato, int superficie, int pista) {
    for (int sec = 0; sec < numsectores; ++sec) {
        if (tryWriteSector(plato, superficie, pista, sec)) {
            return true;
        }
    }
    return false;
}

bool DiskManager::tryWriteSector(int plato, int superficie, int pista, int sector) {
    char sectorPath[256];
    getsectorPath(plato, superficie, pista, sector, sectorPath);
    
    if (fs::exists(sectorPath)) {
        return false;
    }
    
    ofstream sectorTXT(sectorPath);
    if (!sectorTXT.is_open()) {
        cerr << "Error: No se pudo abrir el sector para escritura." << endl;
        return false;
    }
    
    sectorTXT << sectorBuffer;
    sectorTXT.close();
    
    // Reset buffer
    sectorBuffer[0] = '\0';
    sectorSize = 0;
    
    return true;
}

bool DiskManager::appendToSectorBuffer(const char* registro, int registroLength) {
    if (sectorSize > 0 && !addNewlineIfNeeded()) {
        return false;
    }
    
    if (registroLength > 512) {
        cerr << "Error: Registro demasiado grande para un sector (" << registroLength << " bytes)." << endl;
        return false;
    }
    
    if (sectorSize + registroLength > 512 && !flushSectorBuffer()) {
        return false;
    }
    
    strcat(sectorBuffer, registro);
    sectorSize += registroLength;
    return true;
}

void DiskManager::leerRegistro(int plato, int superficie, int pista, int sector, char* buffer) const {
    char sectorPath[256];
    getsectorPath(plato, superficie, pista, sector, sectorPath);
    if (fs::exists(sectorPath)) {
        ifstream sectorTXT(sectorPath);
        if (sectorTXT.is_open()) {
            sectorTXT.getline(buffer, 512);
            sectorTXT.close();
        } else {
            cerr << "Error: No se pudo abrir el sector para lectura." << endl;
            buffer[0] = '\0';
        }
    } else {
        cerr << "Error: El sector especificado no existe." << endl;
        buffer[0] = '\0';
    }
}

const char* DiskManager::tipoDato(const char* dato) const {
    if (dato == nullptr || dato[0] == '\0') return "null";

    char* endPtr;
    strtol(dato, &endPtr, 10);
    if (endPtr != dato && *endPtr == '\0') {
        return "int";
    }

    strtof(dato, &endPtr);
    if (endPtr != dato && *endPtr == '\0') {
        return "float";
    }

    if (strlen(dato) == 1) {
        return "char";
    }

    return "string";
}

void DiskManager::esquemaFile(const char* tableName, const char* headers, const char* types) const {
    if (tablaExisteEnEsquema(tableName)) {
        return;
    }
    
    escribirEsquema(tableName, headers, types);
}

bool DiskManager::tablaExisteEnEsquema(const char* tableName) const {
    if (ifstream esquemaTXT(esquemaPath); esquemaTXT.is_open()) {
        char lineBuffer[1024];
        while (esquemaTXT.getline(lineBuffer, sizeof(lineBuffer))) {
            if (strstr(lineBuffer, tableName) != nullptr) {
                return true;
            }
        }
    }
    return false;
}

void DiskManager::escribirEsquema(const char* tableName, const char* headers, const char* types) const {
    ofstream esquemaOut(esquemaPath, ios::app);
    if (!esquemaOut.is_open()) {
        cerr << "Error: No se pudo abrir esquema.txt." << endl;
        return;
    }
    
    esquemaOut << tableName << " # ";
    procesarCamposEsquema(esquemaOut, headers, types);
    esquemaOut << "\n";
    esquemaOut.close();
}

void DiskManager::procesarCamposEsquema(std::ofstream& esquemaOut, const char* headers, const char* types) const {
    int i = 0;
    int j = 0;
    bool inHeader = true;

    while (headers[i] != '\0' || types[j] != '\0') {
        procesarCampoIndividual(esquemaOut, headers, types, i, j, inHeader);
        
        if (headers[i] != '\0' || types[j] != '\0') {
            esquemaOut << " # ";
        }
    }
}

void DiskManager::procesarCampoIndividual(std::ofstream& esquemaOut, const char* headers, const char* types, 
                                        int& i, int& j, bool& inHeader) const {
    char campo[128] = "";
    int k = 0;

    while ((inHeader ? headers[i] : types[j]) != ',' &&
           (inHeader ? headers[i] : types[j]) != '\0') {
        campo[k++] = inHeader ? headers[i++] : types[j++];
    }

    campo[k] = '\0';
    esquemaOut << campo;

    if ((inHeader ? headers[i] : types[j]) == ',') {
        inHeader ? ++i : ++j;
    }

    inHeader = !inHeader;

    if (headers[i] == '\0' && inHeader) {
        inHeader = false;
    }
}

void DiskManager::dividirCamposCSV(const char* linea, char campos[MAX_CAMPOS][MAX_CAMPO], int& cantidad) const {
    cantidad = 0;
    bool entreComillas = false;
    int j = 0;
    for (int i = 0; linea[i] != '\0' && linea[i] != '\n'; ++i) {
        char c = linea[i];

        if (c == '"') {
            entreComillas = !entreComillas;
        } else if (c == ',' && !entreComillas) {
            campos[cantidad][j] = '\0';
            cantidad++;
            j = 0;
        } else {
            if (j < MAX_CAMPO - 1) {
                campos[cantidad][j++] = c;
            }
        }
    }
    campos[cantidad][j] = '\0';
    cantidad++;
}

void DiskManager::cargarCSV(const char* csvFilePath) {
    ifstream csvFile(csvFilePath);
    if (!csvFile.is_open()) {
        cerr << "Error: No se pudo abrir el archivo CSV." << endl;
        return;
    }

    char encabezados[MAX_CAMPOS][MAX_CAMPO];
    char tipos[MAX_CAMPOS][MAX_CAMPO];
    int numCampos = 0;

    if (!leerYProcesarEncabezados(csvFile, encabezados, tipos, numCampos)) {
        return;
    }

    inferirTiposDatos(csvFile, tipos, numCampos);
    procesarRegistrosCSV(csvFile, numCampos);
    
    guardarEsquema(csvFilePath, encabezados, tipos, numCampos);
    csvFile.close();
}

bool DiskManager::leerYProcesarEncabezados(std::ifstream& csvFile, char encabezados[][MAX_CAMPO], char tipos[][MAX_CAMPO], int& numCampos) const {
    char lineaBuffer[1024];
    if (!csvFile.getline(lineaBuffer, sizeof(lineaBuffer))) {
        cerr << "Error: El archivo CSV está vacío." << endl;
        return false;
    }

    dividirCamposCSV(lineaBuffer, encabezados, numCampos);

    for (int idx = 0; idx < numCampos; idx++) {
        strncpy(tipos[idx], "null", MAX_CAMPO - 1);
        tipos[idx][MAX_CAMPO - 1] = '\0';
    }
    
    return true;
}

void DiskManager::inferirTiposDatos(std::ifstream& csvFile, char tipos[][MAX_CAMPO], 
                                  int numCampos) const {
    const int MAX_LINEAS_TIPO = 10;
    char lineaBuffer[1024];
    char valores[MAX_CAMPOS][MAX_CAMPO];
    int lineasLeidas = 0;

    while (lineasLeidas < MAX_LINEAS_TIPO && csvFile.getline(lineaBuffer, sizeof(lineaBuffer))) {
        procesarLineaParaInferencia(lineaBuffer, tipos, numCampos, valores);
        lineasLeidas++;
    }
}

void DiskManager::procesarLineaParaInferencia(const char* lineaBuffer, char tipos[][MAX_CAMPO], 
                                           int numCampos, char valores[][MAX_CAMPO]) const {
    int numValores = 0;
    dividirCamposCSV(lineaBuffer, valores, numValores);

    for (int idx = 0; idx < numValores && idx < numCampos; idx++) {
        if (strcmp(tipos[idx], "null") == 0) {
            actualizarTipoSiEsNecesario(tipos[idx], valores[idx]);
        }
    }
}

void DiskManager::actualizarTipoSiEsNecesario(char* tipoActual, const char* valor) const {
    const char* nuevoTipo = tipoDato(valor);
    if (strcmp(nuevoTipo, "null") != 0) {
        strncpy(tipoActual, nuevoTipo, MAX_CAMPO - 1);
        tipoActual[MAX_CAMPO - 1] = '\0';
    }
}

void DiskManager::procesarRegistrosCSV(std::ifstream& csvFile, [[maybe_unused]] int numCampos) {
    csvFile.clear();
    csvFile.seekg(0, std::ios::beg);
    
    char lineaBuffer[1024];
    char valores[MAX_CAMPOS][MAX_CAMPO];
    
    csvFile.getline(lineaBuffer, sizeof(lineaBuffer));

    while (csvFile.getline(lineaBuffer, sizeof(lineaBuffer))) {
        procesarLineaCSV(lineaBuffer);
    }
}

void DiskManager::procesarLineaCSV(const char* lineaBuffer) {
    char valores[MAX_CAMPOS][MAX_CAMPO];
    int numValores = 0;
    
    dividirCamposCSV(lineaBuffer, valores, numValores);

    char registroBuffer[1024] = "";
    for (int idx = 0; idx < numValores; idx++) {
        strcat(registroBuffer, valores[idx]);
        if (idx < numValores - 1) strcat(registroBuffer, " # ");
    }

    if (!writeRegistro(registroBuffer)) {
        std::cerr << "Error: No se pudo escribir un registro en el disco." << std::endl;
    }
}

void DiskManager::guardarEsquema(const char* csvFilePath, char encabezados[][MAX_CAMPO], char tipos[][MAX_CAMPO], int numCampos) {
    const char* nombreTabla = strrchr(csvFilePath, '/');
    if (nombreTabla != nullptr) {
        ++nombreTabla;
    } else {
        nombreTabla = csvFilePath;
    }

    char nombreSinExtension[64];
    strncpy(nombreSinExtension, nombreTabla, sizeof(nombreSinExtension));
    nombreSinExtension[sizeof(nombreSinExtension) - 1] = '\0';
    
    if (char* punto = strrchr(nombreSinExtension, '.'); punto && strcmp(punto, ".csv") == 0) {
        *punto = '\0';
    }

    char encabezadosPlano[1024] = "";
    char tiposPlano[1024] = "";
    
    for (int idx = 0; idx < numCampos; idx++) {
        strcat(encabezadosPlano, encabezados[idx]);
        strcat(tiposPlano, tipos[idx]);
        if (idx < numCampos - 1) {
            strcat(encabezadosPlano, ",");
            strcat(tiposPlano, ",");
        }
    }

    esquemaFile(nombreSinExtension, encabezadosPlano, tiposPlano);
}

void DiskManager::mostrarSector(int plato, int superficie, int pista, int sector) const {
    char buffer[512];
    leerRegistro(plato, superficie, pista, sector, buffer);
    cout << "Contenido del sector [" << plato << "][" << superficie << "][" << pista << "][" << sector << "]:" << endl;
    cout << buffer << endl;
}
