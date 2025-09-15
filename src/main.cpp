#include <iostream>
#include "DiskManager.h"

//g++ main.cpp DiskManager.cpp -o gestor

using namespace std;

void displayMenu() {
    cout << "1. Crear Disco" << endl;
    cout << "2. Cargar Archivo" << endl;
    cout << "3. Consulta" << endl;
    cout << "4. Limpiar Disco" << endl;
    cout << "5. Informacion del Disco" << endl;
    cout << "6. Mostrar contenido de un sector" << endl;
    cout << "0. Cerrar" << endl;
    cout << "Seleccione una opcion: ";
}

int main() {
    DiskManager disk("Disk");
    disk.loadConfig();

    int opc;
    do {
        displayMenu();
        cin >> opc;

        switch (opc) {
            case 0:
                cout << "quit..." << endl;
                break;
            case 1:
                disk.diskEstructura();
                //cout << "Disco creado exitosamente." << endl;
                break;
            case 2: {
                char csvFile[100];
                cout << "Ingrese el nombre del archivo CSV: ";
                cin >> csvFile;
                disk.cargarCSV(csvFile);
                break;
            }
            case 3:
                //sql();
                break;
            case 4:
                //clear();
                break;
            case 5:
                //disk.info();
                break;
            case 6: {
                int plato, superficie, pista, sector;
                cout << "Plato Superficie Pista Sector: ";
                cin >> plato >> superficie >> pista >> sector;
                disk.mostrarSector(plato, superficie, pista, sector);
                break;
            }
            default:
                cout << "Opcion incorrecta. Intente de nuevo." << endl;
        }
    } while (opc != 0);

    return 0;
}
