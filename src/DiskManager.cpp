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
    //sectorBuffer[0] = '\0'; //inicializado en la clase

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


// Función writeRegistro simplificada
bool DiskManager::writeRegistro(const char* registro) {
    int registroLength = strlen(registro);
    int espacioNecesario = registroLength + (sectorSize > 0 ? 1 : 0);
    
    if (sectorSize + espacioNecesario > 512) {
        if (!flushSectorBuffer()) {
            return false;
        }
    }
    
    return appendToSectorBuffer(registro, registroLength);
}

// Función helper 1
bool DiskManager::flushSectorBuffer() {
    for (int p = 0; p < numplatos; ++p) {
        for (int s = 0; s < numsuperficies; ++s) {
            for (int t = 0; t < numpistas; ++t) {
                for (int sec = 0; sec < numsectores; ++sec) {
                    if (tryWriteSector(p, s, t, sec)) {
                        return true;
                    }
                }
            }
        }
    }
    
    cerr << "Error: No hay sectores disponibles." << endl;
    return false;
}

// Función helper 2
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

// Función helper 3
bool DiskManager::appendToSectorBuffer(const char* registro, int registroLength) {
    if (sectorSize > 0) {
        if (sectorSize + 1 >= 512) {
            if (!flushSectorBuffer()) {
                return false;
            }
        } else {
            strcat(sectorBuffer, "\n");
            sectorSize += 1;
        }
    }
    
    if (registroLength > 512) {
        cerr << "Error: Registro demasiado grande para un sector (" << registroLength << " bytes)." << endl;
        return false;
    }
    
    if (sectorSize + registroLength > 512) {
        if (!flushSectorBuffer()) {
            return false;
        }
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

    // Verificar si es entero
    char* endPtr;
    long intValue = strtol(dato, &endPtr, 10);
    if (endPtr != dato && *endPtr == '\0') {
        return "int";
    }

    // Verificar si es float
    endPtr = nullptr;
    float floatValue = strtof(dato, &endPtr);
    if (endPtr != dato && *endPtr == '\0') {
        return "float";
    }

    // Verificar si es char (solo un carácter)
    if (strlen(dato) == 1) {
        return "char";
    }

    // Por defecto, string
    return "string";
}

void DiskManager::esquemaFile(const char* tableName, const char* headers, const char* types) {
    ifstream esquemaTXT(esquemaPath);
    if (esquemaTXT.is_open()) {
        char lineBuffer[1024];
        while (esquemaTXT.getline(lineBuffer, sizeof(lineBuffer))) {
            if (strstr(lineBuffer, tableName) != nullptr) {
                esquemaTXT.close();
                return; // La tabla ya existe en el esquema
            }
        }
        esquemaTXT.close();
    }
    /*
    cout << "--------------------------------------------------------------------------------------" << endl;
    cout << "[DEBUG]Encabezados: " << headers << endl;
    cout << "[DEBUG]Tipos: " << types << endl;
    cout << "--------------------------------------------------------------------------------------" << endl;
    */
    ofstream esquemaOut(esquemaPath, ios::app);
    if (esquemaOut.is_open()) {
        esquemaOut << tableName << " # ";

        int i = 0, j = 0;
        bool inHeader = true; // Alternamos entre encabezados y tipos

        while (headers[i] != '\0' || types[j] != '\0') {
            char campo[128] = "";
            int k = 0;

            // Leer un campo del encabezado o tipo
            while ((inHeader ? headers[i] : types[j]) != ',' &&
                   (inHeader ? headers[i] : types[j]) != '\0') {
                campo[k++] = inHeader ? headers[i++] : types[j++];
            }

            campo[k] = '\0'; // Terminar el campo

            // Escribir el campo en el archivo
            esquemaOut << campo;

            // Avanzar al siguiente campo
            if ((inHeader ? headers[i] : types[j]) == ',') {
                inHeader ? ++i : ++j; // Saltar la coma
            }

            // Alternar entre encabezados y tipos
            inHeader = !inHeader;

            // Si aún quedan campos por procesar, escribir " # "
            if (headers[i] != '\0' || types[j] != '\0') {
                esquemaOut << " # ";
            }

            // Si hemos terminado con los encabezados, avanzar al final de los tipos
            if (headers[i] == '\0' && inHeader) {
                inHeader = false;
            }
        }

        esquemaOut << "\n";
        esquemaOut.close();
    } else {
        cerr << "Error: No se pudo abrir esquema.txt." << endl;
    }
}

void DiskManager::cargarCSV(const char* csvFilePath) {
    ifstream csvFile(csvFilePath);
    if (!csvFile.is_open()) {
        cerr << "Error: No se pudo abrir el archivo CSV." << endl;
        return;
    }

    const int MAX_LINEA = 1024;
    const int MAX_CAMPOS = 100;
    const int MAX_CAMPO = 128;
    const int MAX_LINEAS_TIPO = 10; // máximo líneas para inferir tipos

    char lineaBuffer[MAX_LINEA];
    char encabezados[MAX_CAMPOS][MAX_CAMPO];
    char tipos[MAX_CAMPOS][MAX_CAMPO];
    int numCampos = 0;

    // Función para dividir una línea CSV respetando comillas
    auto dividirCamposCSV = [](const char* linea, char campos[MAX_CAMPOS][MAX_CAMPO], int& cantidad) {
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
    };

    // Leer encabezados
    if (!csvFile.getline(lineaBuffer, sizeof(lineaBuffer))) {
        cerr << "Error: El archivo CSV está vacío." << endl;
        return;
    }

    dividirCamposCSV(lineaBuffer, encabezados, numCampos);

    // Inicializar tipos a "null"
    for (int i = 0; i < numCampos; i++) {
        strncpy(tipos[i], "null", MAX_CAMPO - 1);
        tipos[i][MAX_CAMPO - 1] = '\0';
    }

    int lineasLeidasParaTipo = 0;
    char valores[MAX_CAMPOS][MAX_CAMPO];
    bool finArchivoInferencia = false;

    // Leer líneas para inferir tipos
    while (lineasLeidasParaTipo < MAX_LINEAS_TIPO && !finArchivoInferencia) {
        if (!csvFile.getline(lineaBuffer, sizeof(lineaBuffer))) {
            finArchivoInferencia = true; // fin de archivo antes de 10 líneas
            break;
        }

        int numValores = 0;
        dividirCamposCSV(lineaBuffer, valores, numValores);

        for (int i = 0; i < numValores && i < numCampos; i++) {
            // Solo actualizar tipo si actualmente es "null"
            if (strcmp(tipos[i], "null") == 0) {
                const char* nuevoTipo = tipoDato(valores[i]);
                if (strcmp(nuevoTipo, "null") != 0) {
                    strncpy(tipos[i], nuevoTipo, MAX_CAMPO - 1);
                    tipos[i][MAX_CAMPO - 1] = '\0';
                }
            }
        }
        lineasLeidasParaTipo++;
    }

    // Reiniciar lectura para procesar todos los datos desde el principio
    csvFile.clear();
    csvFile.seekg(0, ios::beg);

    // Leer y descartar encabezado (ya lo tenemos)
    csvFile.getline(lineaBuffer, sizeof(lineaBuffer));

    // Procesar y escribir registros
    while (csvFile.getline(lineaBuffer, sizeof(lineaBuffer))) {
        int numValores = 0;
        dividirCamposCSV(lineaBuffer, valores, numValores);

        char registroBuffer[1024] = "";
        for (int i = 0; i < numValores; i++) {
            strcat(registroBuffer, valores[i]);
            if (i < numValores - 1) strcat(registroBuffer, " # ");
        }

        if (!writeRegistro(registroBuffer)) {
            cerr << "Error: No se pudo escribir un registro en el disco." << endl;
            break;
        }
    }

    // Obtener nombre de la tabla desde el archivo CSV
    const char* nombreTabla = strrchr(csvFilePath, '/');
    if (nombreTabla != nullptr) {
        ++nombreTabla;
    } else {
        nombreTabla = csvFilePath;
    }

    // Quitar extensión si tiene .csv
    char nombreSinExtension[64];
    strncpy(nombreSinExtension, nombreTabla, sizeof(nombreSinExtension));
    nombreSinExtension[sizeof(nombreSinExtension) - 1] = '\0';
    char* punto = strrchr(nombreSinExtension, '.');
    if (punto && strcmp(punto, ".csv") == 0) *punto = '\0';

    // Armar cadenas separadas por coma para encabezados y tipos
    char encabezadosPlano[1024] = "";
    char tiposPlano[1024] = "";
    for (int i = 0; i < numCampos; i++) {
        strcat(encabezadosPlano, encabezados[i]);
        strcat(tiposPlano, tipos[i]);
        if (i < numCampos - 1) {
            strcat(encabezadosPlano, ",");
            strcat(tiposPlano, ",");
        }
    }
    /*
    cout << "--------------------------------------------------------------------------------------" << endl;
    cout << "[DEBUG]Encabezados: " << encabezadosPlano << endl;
    cout << "[DEBUG]Tipos: " << tiposPlano << endl;
    cout << "--------------------------------------------------------------------------------------" << endl;
    */  
    esquemaFile(nombreSinExtension, encabezadosPlano, tiposPlano);
    csvFile.close();
}


void DiskManager::mostrarSector(int plato, int superficie, int pista, int sector) const {
    char buffer[512];
    leerRegistro(plato, superficie, pista, sector, buffer);
    cout << "Contenido del sector [" << plato << "][" << superficie << "][" << pista << "][" << sector << "]:" << endl;
    cout << buffer << endl;
}
