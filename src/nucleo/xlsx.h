// Sobres — xlsx.h
// Generador de archivos .xlsx escrito desde cero, sin ninguna biblioteca
// externa.
//
// Un .xlsx es un archivo ZIP con varios documentos XML adentro. Aquí se arma
// ese ZIP guardando las entradas sin comprimir, que es una variante válida del
// formato y ahorra tener que traer una biblioteca de compresión. Excel, Google
// Sheets y LibreOffice lo abren igual.
//
// El módulo no depende de Qt ni del resto de la aplicación: recibe un libro con
// hojas y celdas y devuelve los bytes del archivo.
#pragma once

#include <string>
#include <vector>

#include "dinero.h"
#include "fecha.h"

namespace sobres {
namespace xlsx {

enum class TipoCelda {
  Vacia,
  Texto,
  // Número sin formato especial.
  Numero,
  // Número con formato de moneda: se guarda como número para que Excel pueda
  // sumarlo, no como texto.
  Dinero,
  // Fecha real de Excel, para que se pueda ordenar y filtrar por fecha.
  Fecha,
};

struct Celda {
  TipoCelda tipo = TipoCelda::Vacia;
  std::string texto;
  double numero = 0.0;
  sobres::Fecha fecha;
  bool negrita = false;
};

Celda vacia();
Celda texto(std::string valor, bool negrita = false);
Celda numero(double valor, bool negrita = false);
Celda dinero(Centavos monto, bool negrita = false);
Celda fecha(const sobres::Fecha& f, bool negrita = false);

struct Hoja {
  std::string nombre;
  std::vector<std::vector<Celda>> filas;
  // Ancho de cada columna en caracteres. Se puede dejar vacío.
  std::vector<double> anchosDeColumna;
};

struct Libro {
  std::vector<Hoja> hojas;
};

// Devuelve el contenido binario completo del archivo .xlsx.
std::string generar(const Libro& libro);

// Escribe el libro en disco. Devuelve una cadena vacía si todo salió bien, o
// el problema en texto.
std::string escribirArchivo(const Libro& libro, const std::string& ruta);

// Excel no acepta cualquier nombre de hoja: máximo 31 caracteres, sin los
// caracteres : \ / ? * [ ], y no puede haber dos iguales. Esta función recorta,
// limpia y, si hace falta, agrega un número para no repetir.
std::string nombreDeHojaValido(const std::string& propuesto,
                               const std::vector<std::string>& yaUsados);

// Convierte un número de columna (1) en su letra ("A", "Z", "AA"...).
std::string letraDeColumna(int columna);

// Escapa los caracteres que el XML no admite dentro de un texto.
std::string escaparXml(const std::string& texto);

// Suma de verificación CRC-32 (Cyclic Redundancy Check de 32 bits), la que
// exige el formato ZIP para cada entrada.
std::uint32_t crc32(const std::string& datos);

}  // namespace xlsx
}  // namespace sobres
