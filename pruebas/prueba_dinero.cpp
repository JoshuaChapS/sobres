#include "../src/nucleo/dinero.h"
#include "marco.h"

using namespace sobres;

PRUEBA(dinero_formatea_con_separador_de_miles) {
  VERIFICAR_IGUAL(formatearPesos(123456), std::string("$1,234.56"));
  VERIFICAR_IGUAL(formatearPesos(0), std::string("$0.00"));
  VERIFICAR_IGUAL(formatearPesos(5), std::string("$0.05"));
  VERIFICAR_IGUAL(formatearPesos(100), std::string("$1.00"));
  VERIFICAR_IGUAL(formatearPesos(100000000), std::string("$1,000,000.00"));
}

PRUEBA(dinero_formatea_negativos_y_signos) {
  VERIFICAR_IGUAL(formatearPesos(-123456), std::string("-$1,234.56"));
  VERIFICAR_IGUAL(formatearPesos(123456, true, true), std::string("+$1,234.56"));
  VERIFICAR_IGUAL(formatearPesos(0, true, true), std::string("$0.00"));
  VERIFICAR_IGUAL(formatearPesos(123456, false), std::string("1,234.56"));
}

PRUEBA(dinero_formatea_el_minimo_sin_desbordar) {
  // Negar el mínimo de int64 es comportamiento indefinido si se hace con
  // aritmética con signo; el formateador tiene que sobrevivirlo.
  const std::string texto = formatearPesos(INT64_MIN);
  VERIFICAR(texto.size() > 1);
  VERIFICAR(texto[0] == '-');
}

PRUEBA(dinero_lee_montos_escritos_por_una_persona) {
  VERIFICAR_IGUAL(leerPesos("1234.56").value(), static_cast<Centavos>(123456));
  VERIFICAR_IGUAL(leerPesos("$1,234.56").value(), static_cast<Centavos>(123456));
  VERIFICAR_IGUAL(leerPesos(" 1 234.56 ").value(), static_cast<Centavos>(123456));
  VERIFICAR_IGUAL(leerPesos("12.5").value(), static_cast<Centavos>(1250));
  VERIFICAR_IGUAL(leerPesos("12").value(), static_cast<Centavos>(1200));
  VERIFICAR_IGUAL(leerPesos("-20").value(), static_cast<Centavos>(-2000));
  VERIFICAR_IGUAL(leerPesos(".5").value(), static_cast<Centavos>(50));
}

PRUEBA(dinero_rechaza_lo_que_no_es_un_monto) {
  VERIFICAR(!leerPesos("").has_value());
  VERIFICAR(!leerPesos("abc").has_value());
  VERIFICAR(!leerPesos("1.2.3").has_value());
  VERIFICAR(!leerPesos("1.234").has_value());  // no hay medios centavos
  VERIFICAR(!leerPesos("12-").has_value());
  VERIFICAR(!leerPesos("--12").has_value());
}

PRUEBA(dinero_redondea_alejandose_del_cero) {
  VERIFICAR_IGUAL(redondearCentavos(10.4), static_cast<Centavos>(10));
  VERIFICAR_IGUAL(redondearCentavos(10.5), static_cast<Centavos>(11));
  VERIFICAR_IGUAL(redondearCentavos(-10.5), static_cast<Centavos>(-11));
  VERIFICAR_IGUAL(redondearAPesosEnCentavos(12.345), static_cast<Centavos>(1235));
}

PRUEBA(dinero_ida_y_vuelta_entre_texto_y_centavos) {
  const Centavos montos[] = {0, 1, 99, 100, 12345, -12345, 987654321};
  for (Centavos m : montos) {
    VERIFICAR_IGUAL(leerPesos(formatearPesos(m)).value(), m);
  }
}
