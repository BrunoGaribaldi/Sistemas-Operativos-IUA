#include <stdio.h>
#include <stdlib.h>
#include <string.h>

struct Campo {
    char etiqueta[4]; 
    int longitud; 
    int inicio; 
    char *datos; 
};


void decodificarCampo(char *datosGuardados, struct Campo *campo) {
    campo->datos = (char *)malloc(campo->longitud + 1);
    if (!campo->datos) {
        perror("Error al asignar memoria para los datos del campo");
        exit(1);
    }
    strncpy(campo->datos, datosGuardados + campo->inicio, campo->longitud);
    campo->datos[campo->longitud] = '\0';
}

// Función para imprimir un campo en el formato requerido
void imprimirCampo(struct Campo *campo) {
    if (strcmp(campo->etiqueta, "000") == 0) {
        printf("%s\n", campo->datos); // Campo de control
    } else {
        char *subfield = campo->datos;
        printf("%s ", campo->etiqueta);
        while (*subfield) {
            if (*subfield == 0x1F) { // Subcampo inicia con el delimitador 0x1F
                printf(" $%c ", *(++subfield));
                subfield++;
            } else {
                putchar(*subfield++);
            }
        }
        printf("\n");
    }
}

int main() {
    FILE *file = fopen("osbooks.iso2709", "rb");
    if (!file) {
        perror("Error al abrir el archivo");
        return 1;
    }

    while (1) {
        // Lla cabecera del reg tiene 24 bytes
        char cabecera[25];
        if (fread(cabecera, 1, 24, file) < 24) {
            if (feof(file)) break;
            perror("Error al leer la cabecera del registro");
            fclose(file);
            return 1;
        }
        cabecera[24] = '\0';

        int tamanioRegistro = atoi(cabecera);
        if (tamanioRegistro <= 0) {
            perror("Tamaño de registro inválido");
            fclose(file);
            return 1;
        }
        char *datosGuardados = (char *)malloc(tamanioRegistro - 24);
        if (!datosGuardados) {
            perror("Error al asignar memoria para el registro");
            fclose(file);
            return 1;
        }
        if (fread(datosGuardados, 1, tamanioRegistro - 24, file) < tamanioRegistro - 24) {
            perror("Error al leer el resto del registro");
            free(datosGuardados);
            fclose(file);
            return 1;
        }

        int longitudDirectorio = 0;
        while (datosGuardados[longitudDirectorio] != 0x1E) longitudDirectorio++;
        longitudDirectorio++;

        int cantCampos = longitudDirectorio / 12;
        struct Campo *campos = (struct Campo *)malloc(cantCampos * sizeof(struct Campo));
        if (!campos) {
            perror("Error al asignar memoria para el directorio de campos");
            free(datosGuardados);
            fclose(file);
            return 1;
        }

        for (int i = 0; i < cantCampos; i++) {
            strncpy(campos[i].etiqueta, datosGuardados + i * 12, 3);
            campos[i].etiqueta[3] = '\0';
            campos[i].longitud = atoi(strndup(datosGuardados + i * 12 + 3, 4));
            campos[i].inicio = atoi(strndup(datosGuardados + i * 12 + 7, 5));
            decodificarCampo(datosGuardados, &campos[i]);
        }

        for (int i = 0; i < cantCampos; i++) {
            imprimirCampo(&campos[i]);
            free(campos[i].datos);
        }

        free(campos);
        free(datosGuardados);
    }

    fclose(file);
    return 0;
}