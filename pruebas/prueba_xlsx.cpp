#include "../src/nucleo/xlsx.h"

#include "marco.h"

using namespace sobres;
using namespace sobres::xlsx;

namespace {

bool contiene(const std::string& donde, const std::string& que) {
  return donde.find(que) != std::string::npos;
}

Libro libroDeUnaHoja(const std::vector<std::vector<Celda>>& filas) {
  Hoja hoja;
  hoja.nombre = "Prueba";
  hoja.filas = filas;
  Libro libro;
  libro.hojas.push_back(hoja);
  return libro;
}

}  // namespace

PRUEBA(xlsx_calcula_el_crc32_estandar) {
  // Valor de referencia del estándar: la suma de "123456789" es 0xCBF43926.
  VERIFICAR_IGUAL(crc32("123456789"), static_cast<std::uint32_t>(0xCBF43926));
  VERIFICAR_IGUAL(crc32(""), static_cast<std::uint32_t>(0));
}

PRUEBA(xlsx_nombra_las_columnas_como_excel) {
  VERIFICAR_IGUAL(letraDeColumna(1), std::string("A"));
  VERIFICAR_IGUAL(letraDeColumna(26), std::string("Z"));
  VERIFICAR_IGUAL(letraDeColumna(27), std::string("AA"));
  VERIFICAR_IGUAL(letraDeColumna(28), std::string("AB"));
  VERIFICAR_IGUAL(letraDeColumna(52), std::string("AZ"));
  VERIFICAR_IGUAL(letraDeColumna(702), std::string("ZZ"));
  VERIFICAR_IGUAL(letraDeColumna(703), std::string("AAA"));
}

PRUEBA(xlsx_escapa_lo_que_el_xml_no_admite) {
  VERIFICAR_IGUAL(escaparXml("Luz & agua"), std::string("Luz &amp; agua"));
  VERIFICAR_IGUAL(escaparXml("<b>"), std::string("&lt;b&gt;"));
  VERIFICAR_IGUAL(escaparXml("comilla \" y apóstrofo '"),
                  std::string("comilla &quot; y apóstrofo &apos;"));
  // Los caracteres de control se descartan en lugar de romper el archivo.
  VERIFICAR_IGUAL(escaparXml(std::string("a\x01") + "b"), std::string("ab"));
}

PRUEBA(xlsx_arregla_los_nombres_de_hoja_invalidos) {
  VERIFICAR_IGUAL(nombreDeHojaValido("Gasto diario", {}),
                  std::string("Gasto diario"));
  // Excel no acepta estos caracteres en el nombre de una hoja.
  VERIFICAR_IGUAL(nombreDeHojaValido("Casa/servicios[2]", {}),
                  std::string("Casa servicios 2 "));
  // Máximo 31 caracteres.
  const std::string largo(50, 'x');
  VERIFICAR_IGUAL(nombreDeHojaValido(largo, {}).size(),
                  static_cast<std::size_t>(31));
  // Y no puede haber dos hojas con el mismo nombre.
  VERIFICAR_IGUAL(nombreDeHojaValido("Ahorro", {"Ahorro"}),
                  std::string("Ahorro 2"));
  VERIFICAR_IGUAL(nombreDeHojaValido("Ahorro", {"Ahorro", "Ahorro 2"}),
                  std::string("Ahorro 3"));
  VERIFICAR(!nombreDeHojaValido("", {}).empty());
}

PRUEBA(xlsx_produce_un_zip_con_la_estructura_esperada) {
  const std::string archivo = generar(libroDeUnaHoja({{texto("Hola")}}));

  // Firma de encabezado local al principio y de fin de directorio al final.
  VERIFICAR(archivo.size() > 22);
  VERIFICAR_IGUAL(archivo.substr(0, 4), std::string("PK\x03\x04", 4));
  VERIFICAR(contiene(archivo, std::string("PK\x05\x06", 4)));
  VERIFICAR(contiene(archivo, std::string("PK\x01\x02", 4)));

  // Las seis partes que exige el formato con una sola hoja.
  VERIFICAR(contiene(archivo, "[Content_Types].xml"));
  VERIFICAR(contiene(archivo, "_rels/.rels"));
  VERIFICAR(contiene(archivo, "xl/workbook.xml"));
  VERIFICAR(contiene(archivo, "xl/_rels/workbook.xml.rels"));
  VERIFICAR(contiene(archivo, "xl/styles.xml"));
  VERIFICAR(contiene(archivo, "xl/worksheets/sheet1.xml"));
}

PRUEBA(xlsx_guarda_el_dinero_como_numero_y_no_como_texto) {
  // Es la diferencia entre poder sumar la columna en Excel y no poder.
  const std::string archivo =
      generar(libroDeUnaHoja({{dinero(123456), dinero(-5000), dinero(0)}}));
  VERIFICAR(contiene(archivo, "<v>1234.56</v>"));
  VERIFICAR(contiene(archivo, "<v>-50.00</v>"));
  VERIFICAR(contiene(archivo, "<v>0.00</v>"));
  // Y con el formato de moneda aplicado.
  VERIFICAR(contiene(archivo, "&quot;$&quot;#,##0.00"));
}

PRUEBA(xlsx_guarda_las_fechas_como_fechas_de_excel) {
  const Fecha f{2026, 9, 4};
  const std::string archivo = generar(libroDeUnaHoja({{fecha(f)}}));
  // El día 0 de Excel es el 30 de diciembre de 1899, 25569 días antes del 1 de
  // enero de 1970.
  const long long esperado = aDiasSeriales(f) + 25569;
  VERIFICAR(contiene(archivo, "<v>" + std::to_string(esperado) + "</v>"));
  VERIFICAR(contiene(archivo, "dd/mm/yyyy"));
}

PRUEBA(xlsx_escribe_el_texto_dentro_de_la_celda) {
  const std::string archivo =
      generar(libroDeUnaHoja({{texto("Luz & agua", true)}}));
  VERIFICAR(contiene(archivo, "t=\"inlineStr\""));
  VERIFICAR(contiene(archivo, "Luz &amp; agua"));
  // La celda en negrita usa el estilo 1.
  VERIFICAR(contiene(archivo, "s=\"1\""));
}

PRUEBA(xlsx_omite_las_celdas_vacias_pero_respeta_las_columnas) {
  const std::string archivo =
      generar(libroDeUnaHoja({{texto("A"), vacia(), texto("C")}}));
  VERIFICAR(contiene(archivo, "r=\"A1\""));
  VERIFICAR(contiene(archivo, "r=\"C1\""));
  VERIFICAR(!contiene(archivo, "r=\"B1\""));
}

PRUEBA(xlsx_registra_todas_las_hojas_del_libro) {
  Libro libro;
  for (const char* nombre : {"Resumen", "Gasto diario", "Casa"}) {
    Hoja hoja;
    hoja.nombre = nombre;
    hoja.filas = {{texto(nombre)}};
    libro.hojas.push_back(hoja);
  }
  const std::string archivo = generar(libro);

  VERIFICAR(contiene(archivo, "xl/worksheets/sheet1.xml"));
  VERIFICAR(contiene(archivo, "xl/worksheets/sheet2.xml"));
  VERIFICAR(contiene(archivo, "xl/worksheets/sheet3.xml"));
  VERIFICAR(contiene(archivo, "<sheet name=\"Gasto diario\" sheetId=\"2\""));
  // La relación de estilos va después de las tres hojas.
  VERIFICAR(contiene(archivo, "Id=\"rId4\""));
}

PRUEBA(xlsx_es_igual_byte_a_byte_en_dos_ejecuciones) {
  // Sin esto, dos reportes del mismo mes se verían distintos solo por la hora
  // en que se generaron.
  const Libro libro = libroDeUnaHoja({{texto("A"), dinero(100)}});
  VERIFICAR_IGUAL(generar(libro).size(), generar(libro).size());
  VERIFICAR(generar(libro) == generar(libro));
}

PRUEBA(xlsx_se_niega_a_escribir_un_libro_sin_hojas) {
  Libro vacio;
  VERIFICAR(!escribirArchivo(vacio, "/tmp/no-deberia-existir.xlsx").empty());
}
