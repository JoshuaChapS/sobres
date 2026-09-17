#include "../src/nucleo/fecha.h"
#include "marco.h"

using namespace sobres;

PRUEBA(fecha_conoce_los_bisiestos) {
  VERIFICAR(esBisiesto(2024));
  VERIFICAR(esBisiesto(2000));
  VERIFICAR(!esBisiesto(1900));
  VERIFICAR(!esBisiesto(2026));
  VERIFICAR_IGUAL(diasDelMes(2024, 2), 29);
  VERIFICAR_IGUAL(diasDelMes(2026, 2), 28);
}

PRUEBA(fecha_convierte_a_dias_seriales_y_de_regreso) {
  VERIFICAR_IGUAL(aDiasSeriales(Fecha{1970, 1, 1}), 0LL);
  VERIFICAR_IGUAL(aDiasSeriales(Fecha{1970, 1, 2}), 1LL);
  VERIFICAR_IGUAL(aDiasSeriales(Fecha{1969, 12, 31}), -1LL);
  VERIFICAR_IGUAL(aDiasSeriales(Fecha{2000, 3, 1}), 11017LL);

  for (long long d = -40000; d <= 40000; d += 137) {
    VERIFICAR_IGUAL(aDiasSeriales(desdeDiasSeriales(d)), d);
  }
}

PRUEBA(fecha_suma_dias_cruzando_meses_y_anios) {
  VERIFICAR(sumarDias(Fecha{2026, 12, 31}, 1) == (Fecha{2027, 1, 1}));
  VERIFICAR(sumarDias(Fecha{2024, 2, 28}, 1) == (Fecha{2024, 2, 29}));
  VERIFICAR(sumarDias(Fecha{2026, 2, 28}, 1) == (Fecha{2026, 3, 1}));
  VERIFICAR(sumarDias(Fecha{2026, 1, 1}, -1) == (Fecha{2025, 12, 31}));
}

PRUEBA(fecha_suma_meses_recortando_el_dia) {
  VERIFICAR(sumarMeses(Fecha{2026, 1, 31}, 1) == (Fecha{2026, 2, 28}));
  VERIFICAR(sumarMeses(Fecha{2024, 1, 31}, 1) == (Fecha{2024, 2, 29}));
  VERIFICAR(sumarMeses(Fecha{2026, 1, 15}, 12) == (Fecha{2027, 1, 15}));
  VERIFICAR(sumarMeses(Fecha{2026, 3, 15}, -3) == (Fecha{2025, 12, 15}));
  VERIFICAR(sumarMeses(Fecha{2026, 1, 15}, -1) == (Fecha{2025, 12, 15}));
}

PRUEBA(fecha_encuentra_los_bordes_del_mes) {
  VERIFICAR(primerDiaDelMes(Fecha{2026, 9, 4}) == (Fecha{2026, 9, 1}));
  VERIFICAR(ultimoDiaDelMes(Fecha{2026, 9, 4}) == (Fecha{2026, 9, 30}));
  VERIFICAR(ultimoDiaDelMes(Fecha{2024, 2, 1}) == (Fecha{2024, 2, 29}));
}

PRUEBA(fecha_va_y_viene_del_texto_iso) {
  VERIFICAR_IGUAL(aTextoIso(Fecha{2026, 9, 4}), std::string("2026-09-04"));
  VERIFICAR(desdeTextoIso("2026-09-04").value() == (Fecha{2026, 9, 4}));
  VERIFICAR(!desdeTextoIso("2026-13-01").has_value());
  VERIFICAR(!desdeTextoIso("2026-02-30").has_value());
  VERIFICAR(!desdeTextoIso("04/09/2026").has_value());
  VERIFICAR(!desdeTextoIso("2026-9-4").has_value());
}

PRUEBA(fecha_se_ordena_cronologicamente) {
  VERIFICAR(Fecha{2026, 1, 2} < (Fecha{2026, 2, 1}));
  VERIFICAR(Fecha{2025, 12, 31} < (Fecha{2026, 1, 1}));
  VERIFICAR(!(Fecha{2026, 1, 1} < (Fecha{2026, 1, 1})));
  VERIFICAR_IGUAL(diasEntre(Fecha{2026, 1, 1}, Fecha{2026, 12, 31}), 364LL);
}

PRUEBA(fecha_escribe_texto_en_espaniol) {
  VERIFICAR_IGUAL(aTextoLargo(Fecha{2026, 9, 4}),
                  std::string("4 de septiembre de 2026"));
  VERIFICAR_IGUAL(nombreDeMes(12), std::string("diciembre"));
}
