#include "xlsx.h"

#include <algorithm>
#include <array>
#include <cstdio>
#include <fstream>

namespace sobres {
namespace xlsx {
namespace {

// Índices de estilo tal como quedan definidos en styles.xml, más abajo.
constexpr int kEstiloNormal = 0;
constexpr int kEstiloNegrita = 1;
constexpr int kEstiloDinero = 2;
constexpr int kEstiloFecha = 3;
constexpr int kEstiloDineroNegrita = 4;

// El día 0 del calendario de Excel es el 30 de diciembre de 1899, y el 1 de
// enero de 1970 (día 0 del calendario del núcleo) cae en el 25569.
constexpr long long kDesfaseDeFechaDeExcel = 25569;

// Fecha y hora fijas para las entradas del ZIP. Que no dependan del reloj hace
// que dos ejecuciones con los mismos datos produzcan el mismo archivo, lo que
// facilita comprobarlo en las pruebas.
constexpr std::uint16_t kHoraDos = 0;
constexpr std::uint16_t kFechaDos = ((2020 - 1980) << 9) | (1 << 5) | 1;

void agregar16(std::string& destino, std::uint16_t valor) {
  destino.push_back(static_cast<char>(valor & 0xff));
  destino.push_back(static_cast<char>((valor >> 8) & 0xff));
}

void agregar32(std::string& destino, std::uint32_t valor) {
  destino.push_back(static_cast<char>(valor & 0xff));
  destino.push_back(static_cast<char>((valor >> 8) & 0xff));
  destino.push_back(static_cast<char>((valor >> 16) & 0xff));
  destino.push_back(static_cast<char>((valor >> 24) & 0xff));
}

std::string formatear(double valor, const char* formato) {
  char buffer[64];
  std::snprintf(buffer, sizeof(buffer), formato, valor);
  return std::string(buffer);
}

int estiloDe(const Celda& celda) {
  switch (celda.tipo) {
    case TipoCelda::Dinero:
      return celda.negrita ? kEstiloDineroNegrita : kEstiloDinero;
    case TipoCelda::Fecha:
      return kEstiloFecha;
    case TipoCelda::Vacia:
    case TipoCelda::Texto:
    case TipoCelda::Numero:
      break;
  }
  return celda.negrita ? kEstiloNegrita : kEstiloNormal;
}

struct Entrada {
  std::string nombre;
  std::string contenido;
};

// Arma el ZIP con todas las entradas guardadas sin comprimir.
std::string empaquetarZip(const std::vector<Entrada>& entradas) {
  std::string archivo;
  std::string directorio;
  std::vector<std::uint32_t> desplazamientos;
  desplazamientos.reserve(entradas.size());

  for (const Entrada& entrada : entradas) {
    desplazamientos.push_back(static_cast<std::uint32_t>(archivo.size()));
    const std::uint32_t suma = crc32(entrada.contenido);
    const std::uint32_t tamanio =
        static_cast<std::uint32_t>(entrada.contenido.size());

    agregar32(archivo, 0x04034b50);  // firma de encabezado local
    agregar16(archivo, 20);          // versión necesaria
    agregar16(archivo, 0);           // banderas
    agregar16(archivo, 0);           // método: guardado sin comprimir
    agregar16(archivo, kHoraDos);
    agregar16(archivo, kFechaDos);
    agregar32(archivo, suma);
    agregar32(archivo, tamanio);  // tamaño comprimido
    agregar32(archivo, tamanio);  // tamaño original
    agregar16(archivo, static_cast<std::uint16_t>(entrada.nombre.size()));
    agregar16(archivo, 0);  // sin campo extra
    archivo += entrada.nombre;
    archivo += entrada.contenido;
  }

  const std::uint32_t inicioDirectorio =
      static_cast<std::uint32_t>(archivo.size());
  for (std::size_t k = 0; k < entradas.size(); ++k) {
    const Entrada& entrada = entradas[k];
    const std::uint32_t suma = crc32(entrada.contenido);
    const std::uint32_t tamanio =
        static_cast<std::uint32_t>(entrada.contenido.size());

    agregar32(directorio, 0x02014b50);  // firma de entrada del directorio
    agregar16(directorio, 20);          // versión con la que se creó
    agregar16(directorio, 20);          // versión necesaria
    agregar16(directorio, 0);
    agregar16(directorio, 0);
    agregar16(directorio, kHoraDos);
    agregar16(directorio, kFechaDos);
    agregar32(directorio, suma);
    agregar32(directorio, tamanio);
    agregar32(directorio, tamanio);
    agregar16(directorio, static_cast<std::uint16_t>(entrada.nombre.size()));
    agregar16(directorio, 0);  // extra
    agregar16(directorio, 0);  // comentario
    agregar16(directorio, 0);  // disco donde empieza
    agregar16(directorio, 0);  // atributos internos
    agregar32(directorio, 0);  // atributos externos
    agregar32(directorio, desplazamientos[k]);
    directorio += entrada.nombre;
  }

  archivo += directorio;
  agregar32(archivo, 0x06054b50);  // fin del directorio central
  agregar16(archivo, 0);
  agregar16(archivo, 0);
  agregar16(archivo, static_cast<std::uint16_t>(entradas.size()));
  agregar16(archivo, static_cast<std::uint16_t>(entradas.size()));
  agregar32(archivo, static_cast<std::uint32_t>(directorio.size()));
  agregar32(archivo, inicioDirectorio);
  agregar16(archivo, 0);  // sin comentario
  return archivo;
}

const char* kEncabezadoXml =
    "<?xml version=\"1.0\" encoding=\"UTF-8\" standalone=\"yes\"?>\n";

std::string tiposDeContenido(std::size_t cuantasHojas) {
  std::string xml = kEncabezadoXml;
  xml +=
      "<Types xmlns=\"http://schemas.openxmlformats.org/package/2006/"
      "content-types\">";
  xml +=
      "<Default Extension=\"rels\" ContentType=\"application/"
      "vnd.openxmlformats-package.relationships+xml\"/>";
  xml += "<Default Extension=\"xml\" ContentType=\"application/xml\"/>";
  xml +=
      "<Override PartName=\"/xl/workbook.xml\" ContentType=\"application/"
      "vnd.openxmlformats-officedocument.spreadsheetml.sheet.main+xml\"/>";
  for (std::size_t k = 1; k <= cuantasHojas; ++k) {
    xml += "<Override PartName=\"/xl/worksheets/sheet" + std::to_string(k) +
           ".xml\" ContentType=\"application/"
           "vnd.openxmlformats-officedocument.spreadsheetml.worksheet+xml\"/>";
  }
  xml +=
      "<Override PartName=\"/xl/styles.xml\" ContentType=\"application/"
      "vnd.openxmlformats-officedocument.spreadsheetml.styles+xml\"/>";
  xml += "</Types>";
  return xml;
}

std::string relacionesRaiz() {
  std::string xml = kEncabezadoXml;
  xml +=
      "<Relationships xmlns=\"http://schemas.openxmlformats.org/package/2006/"
      "relationships\">";
  xml +=
      "<Relationship Id=\"rId1\" Type=\"http://schemas.openxmlformats.org/"
      "officeDocument/2006/relationships/officeDocument\" "
      "Target=\"xl/workbook.xml\"/>";
  xml += "</Relationships>";
  return xml;
}

std::string libroXml(const std::vector<std::string>& nombresDeHoja) {
  std::string xml = kEncabezadoXml;
  xml +=
      "<workbook xmlns=\"http://schemas.openxmlformats.org/spreadsheetml/2006/"
      "main\" xmlns:r=\"http://schemas.openxmlformats.org/officeDocument/2006/"
      "relationships\"><sheets>";
  for (std::size_t k = 0; k < nombresDeHoja.size(); ++k) {
    xml += "<sheet name=\"" + escaparXml(nombresDeHoja[k]) + "\" sheetId=\"" +
           std::to_string(k + 1) + "\" r:id=\"rId" + std::to_string(k + 1) +
           "\"/>";
  }
  xml += "</sheets></workbook>";
  return xml;
}

std::string relacionesDelLibro(std::size_t cuantasHojas) {
  std::string xml = kEncabezadoXml;
  xml +=
      "<Relationships xmlns=\"http://schemas.openxmlformats.org/package/2006/"
      "relationships\">";
  for (std::size_t k = 1; k <= cuantasHojas; ++k) {
    xml += "<Relationship Id=\"rId" + std::to_string(k) +
           "\" Type=\"http://schemas.openxmlformats.org/officeDocument/2006/"
           "relationships/worksheet\" Target=\"worksheets/sheet" +
           std::to_string(k) + ".xml\"/>";
  }
  xml += "<Relationship Id=\"rId" + std::to_string(cuantasHojas + 1) +
         "\" Type=\"http://schemas.openxmlformats.org/officeDocument/2006/"
         "relationships/styles\" Target=\"styles.xml\"/>";
  xml += "</Relationships>";
  return xml;
}

std::string estilosXml() {
  std::string xml = kEncabezadoXml;
  xml +=
      "<styleSheet xmlns=\"http://schemas.openxmlformats.org/spreadsheetml/"
      "2006/main\">";
  // 164 es moneda y 165 fecha; los identificadores propios empiezan en 164 por
  // convención del formato.
  xml +=
      "<numFmts count=\"2\">"
      "<numFmt numFmtId=\"164\" formatCode=\"&quot;$&quot;#,##0.00\"/>"
      "<numFmt numFmtId=\"165\" formatCode=\"dd/mm/yyyy\"/>"
      "</numFmts>";
  xml +=
      "<fonts count=\"2\">"
      "<font><sz val=\"11\"/><name val=\"Calibri\"/></font>"
      "<font><b/><sz val=\"11\"/><name val=\"Calibri\"/></font>"
      "</fonts>";
  xml +=
      "<fills count=\"2\">"
      "<fill><patternFill patternType=\"none\"/></fill>"
      "<fill><patternFill patternType=\"gray125\"/></fill>"
      "</fills>";
  xml +=
      "<borders count=\"1\"><border><left/><right/><top/><bottom/><diagonal/>"
      "</border></borders>";
  xml +=
      "<cellStyleXfs count=\"1\"><xf numFmtId=\"0\" fontId=\"0\" fillId=\"0\" "
      "borderId=\"0\"/></cellStyleXfs>";
  xml +=
      "<cellXfs count=\"5\">"
      "<xf numFmtId=\"0\" fontId=\"0\" fillId=\"0\" borderId=\"0\" "
      "xfId=\"0\"/>"
      "<xf numFmtId=\"0\" fontId=\"1\" fillId=\"0\" borderId=\"0\" xfId=\"0\" "
      "applyFont=\"1\"/>"
      "<xf numFmtId=\"164\" fontId=\"0\" fillId=\"0\" borderId=\"0\" "
      "xfId=\"0\" applyNumberFormat=\"1\"/>"
      "<xf numFmtId=\"165\" fontId=\"0\" fillId=\"0\" borderId=\"0\" "
      "xfId=\"0\" applyNumberFormat=\"1\"/>"
      "<xf numFmtId=\"164\" fontId=\"1\" fillId=\"0\" borderId=\"0\" "
      "xfId=\"0\" applyNumberFormat=\"1\" applyFont=\"1\"/>"
      "</cellXfs>";
  xml +=
      "<cellStyles count=\"1\"><cellStyle name=\"Normal\" xfId=\"0\" "
      "builtinId=\"0\"/></cellStyles>";
  xml += "</styleSheet>";
  return xml;
}

std::string hojaXml(const Hoja& hoja) {
  std::string xml = kEncabezadoXml;
  xml +=
      "<worksheet xmlns=\"http://schemas.openxmlformats.org/spreadsheetml/2006/"
      "main\">";

  if (!hoja.anchosDeColumna.empty()) {
    xml += "<cols>";
    for (std::size_t k = 0; k < hoja.anchosDeColumna.size(); ++k) {
      xml += "<col min=\"" + std::to_string(k + 1) + "\" max=\"" +
             std::to_string(k + 1) + "\" width=\"" +
             formatear(hoja.anchosDeColumna[k], "%.2f") +
             "\" customWidth=\"1\"/>";
    }
    xml += "</cols>";
  }

  xml += "<sheetData>";
  for (std::size_t f = 0; f < hoja.filas.size(); ++f) {
    const std::vector<Celda>& fila = hoja.filas[f];
    xml += "<row r=\"" + std::to_string(f + 1) + "\">";
    for (std::size_t c = 0; c < fila.size(); ++c) {
      const Celda& celda = fila[c];
      if (celda.tipo == TipoCelda::Vacia) continue;

      const std::string referencia =
          letraDeColumna(static_cast<int>(c) + 1) + std::to_string(f + 1);
      const int estilo = estiloDe(celda);
      xml += "<c r=\"" + referencia + "\"";
      if (estilo != 0) xml += " s=\"" + std::to_string(estilo) + "\"";

      if (celda.tipo == TipoCelda::Texto) {
        // Los textos van incrustados en la propia celda; así no hace falta la
        // tabla de cadenas compartidas que usa Excel para ahorrar espacio.
        xml += " t=\"inlineStr\"><is><t xml:space=\"preserve\">" +
               escaparXml(celda.texto) + "</t></is></c>";
      } else if (celda.tipo == TipoCelda::Fecha) {
        const long long serie =
            aDiasSeriales(celda.fecha) + kDesfaseDeFechaDeExcel;
        xml += "><v>" + std::to_string(serie) + "</v></c>";
      } else if (celda.tipo == TipoCelda::Dinero) {
        xml += "><v>" + formatear(celda.numero, "%.2f") + "</v></c>";
      } else {
        xml += "><v>" + formatear(celda.numero, "%.10g") + "</v></c>";
      }
    }
    xml += "</row>";
  }
  xml += "</sheetData></worksheet>";
  return xml;
}

}  // namespace

std::uint32_t crc32(const std::string& datos) {
  static std::array<std::uint32_t, 256> tabla;
  static bool lista = false;
  if (!lista) {
    for (std::uint32_t i = 0; i < 256; ++i) {
      std::uint32_t c = i;
      for (int k = 0; k < 8; ++k) {
        c = (c & 1u) ? (0xEDB88320u ^ (c >> 1)) : (c >> 1);
      }
      tabla[i] = c;
    }
    lista = true;
  }
  std::uint32_t suma = 0xFFFFFFFFu;
  for (unsigned char byte : datos) {
    suma = tabla[(suma ^ byte) & 0xFFu] ^ (suma >> 8);
  }
  return suma ^ 0xFFFFFFFFu;
}

std::string escaparXml(const std::string& texto) {
  std::string salida;
  salida.reserve(texto.size());
  for (char c : texto) {
    switch (c) {
      case '&': salida += "&amp;"; break;
      case '<': salida += "&lt;"; break;
      case '>': salida += "&gt;"; break;
      case '"': salida += "&quot;"; break;
      case '\'': salida += "&apos;"; break;
      default:
        // Los caracteres de control no son válidos en XML; se descartan.
        if (static_cast<unsigned char>(c) < 0x20 && c != '\t' && c != '\n' &&
            c != '\r') {
          break;
        }
        salida.push_back(c);
    }
  }
  return salida;
}

std::string letraDeColumna(int columna) {
  std::string letras;
  while (columna > 0) {
    const int resto = (columna - 1) % 26;
    letras.insert(letras.begin(), static_cast<char>('A' + resto));
    columna = (columna - 1) / 26;
  }
  return letras.empty() ? "A" : letras;
}

std::string nombreDeHojaValido(const std::string& propuesto,
                               const std::vector<std::string>& yaUsados) {
  std::string limpio;
  for (char c : propuesto) {
    if (c == ':' || c == '\\' || c == '/' || c == '?' || c == '*' ||
        c == '[' || c == ']') {
      limpio.push_back(' ');
    } else {
      limpio.push_back(c);
    }
  }
  while (!limpio.empty() && limpio.front() == '\'') limpio.erase(limpio.begin());
  while (!limpio.empty() && limpio.back() == '\'') limpio.pop_back();
  if (limpio.empty()) limpio = "Hoja";
  if (limpio.size() > 31) limpio = limpio.substr(0, 31);

  auto yaEsta = [&yaUsados](const std::string& candidato) {
    return std::find(yaUsados.begin(), yaUsados.end(), candidato) !=
           yaUsados.end();
  };
  if (!yaEsta(limpio)) return limpio;

  for (int intento = 2; intento < 1000; ++intento) {
    const std::string sufijo = " " + std::to_string(intento);
    std::string base = limpio;
    if (base.size() + sufijo.size() > 31) {
      base = base.substr(0, 31 - sufijo.size());
    }
    const std::string candidato = base + sufijo;
    if (!yaEsta(candidato)) return candidato;
  }
  return limpio;
}

Celda vacia() { return Celda{}; }

Celda texto(std::string valor, bool negrita) {
  Celda c;
  c.tipo = TipoCelda::Texto;
  c.texto = std::move(valor);
  c.negrita = negrita;
  return c;
}

Celda numero(double valor, bool negrita) {
  Celda c;
  c.tipo = TipoCelda::Numero;
  c.numero = valor;
  c.negrita = negrita;
  return c;
}

Celda dinero(Centavos monto, bool negrita) {
  Celda c;
  c.tipo = TipoCelda::Dinero;
  // Se divide entre 100 solo aquí, al salir hacia la hoja de cálculo. Adentro
  // de la aplicación el monto siguió siendo un entero de centavos hasta este
  // punto.
  c.numero = static_cast<double>(monto) / 100.0;
  c.negrita = negrita;
  return c;
}

Celda fecha(const sobres::Fecha& f, bool negrita) {
  Celda c;
  c.tipo = TipoCelda::Fecha;
  c.fecha = f;
  c.negrita = negrita;
  return c;
}

std::string generar(const Libro& libro) {
  std::vector<Entrada> entradas;
  std::vector<std::string> nombres;
  for (const Hoja& hoja : libro.hojas) nombres.push_back(hoja.nombre);
  const std::size_t cuantas = libro.hojas.size();

  entradas.push_back({"[Content_Types].xml", tiposDeContenido(cuantas)});
  entradas.push_back({"_rels/.rels", relacionesRaiz()});
  entradas.push_back({"xl/workbook.xml", libroXml(nombres)});
  entradas.push_back({"xl/_rels/workbook.xml.rels", relacionesDelLibro(cuantas)});
  entradas.push_back({"xl/styles.xml", estilosXml()});
  for (std::size_t k = 0; k < cuantas; ++k) {
    entradas.push_back({"xl/worksheets/sheet" + std::to_string(k + 1) + ".xml",
                        hojaXml(libro.hojas[k])});
  }
  return empaquetarZip(entradas);
}

std::string escribirArchivo(const Libro& libro, const std::string& ruta) {
  if (libro.hojas.empty()) return "El libro no tiene ninguna hoja.";
  const std::string contenido = generar(libro);
  std::ofstream salida(ruta, std::ios::binary | std::ios::trunc);
  if (!salida) return "No se pudo abrir el archivo para escribir: " + ruta;
  salida.write(contenido.data(),
               static_cast<std::streamsize>(contenido.size()));
  if (!salida) return "No se pudo escribir todo el archivo: " + ruta;
  salida.close();
  return "";
}

}  // namespace xlsx
}  // namespace sobres
