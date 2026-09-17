// Sobres — modelos.h
// Las entidades del dominio. Son structs planos, sin lógica de base de datos y
// sin Qt, para que tanto la capa de datos como la interfaz dependan de ellos y
// no al revés.
#pragma once

#include <string>
#include <vector>

#include "dinero.h"
#include "fecha.h"

namespace sobres {

// Identificador de fila. 0 significa "todavía no está guardado".
using Id = std::int64_t;
inline constexpr Id kSinId = 0;

// ---------------------------------------------------------------------------
// Sobre
// ---------------------------------------------------------------------------

enum class TipoSobre {
  // Dinero que es del usuario y está disponible para gastar.
  Propio = 0,
  // Dinero de terceros que solo está de paso por la cuenta. No es patrimonio.
  Ajeno = 1,
  // Dinero del usuario apartado para una meta. Sí es patrimonio, pero se
  // presenta aparte porque está comprometido.
  Meta = 2,
};

// La regla central de la aplicación: qué sobres cuentan para el patrimonio.
// El dinero ajeno está en la cuenta pero no es del usuario.
inline bool cuentaParaPatrimonio(TipoSobre tipo) {
  return tipo != TipoSobre::Ajeno;
}

std::string nombreDeTipo(TipoSobre tipo);
const char* claveDeTipo(TipoSobre tipo);

struct Sobre {
  Id id = kSinId;
  std::string nombre;
  std::string color = "#6b7280";  // color en formato #RRGGBB
  TipoSobre tipo = TipoSobre::Propio;
  int orden = 0;
  bool archivado = false;
};

// ---------------------------------------------------------------------------
// Categoría
// ---------------------------------------------------------------------------

struct Categoria {
  Id id = kSinId;
  std::string nombre;
  std::string color = "#6b7280";
  std::string icono;  // opcional: un emoji o una letra
  bool archivada = false;
};

// ---------------------------------------------------------------------------
// Movimiento
// ---------------------------------------------------------------------------

enum class TipoMovimiento {
  Entrada = 0,   // dinero que llega a sobreOrigen
  Gasto = 1,     // dinero que sale de sobreOrigen
  Traspaso = 2,  // dinero que va de sobreOrigen a sobreDestino
};

std::string nombreDeTipoMovimiento(TipoMovimiento tipo);

struct Movimiento {
  Id id = kSinId;
  Fecha fecha;
  TipoMovimiento tipo = TipoMovimiento::Gasto;
  // Sobre afectado. En una entrada es el sobre que recibe; en un gasto y en un
  // traspaso es el sobre del que sale el dinero.
  Id sobreOrigen = kSinId;
  // Solo se usa en los traspasos.
  Id sobreDestino = kSinId;
  Id categoria = kSinId;  // opcional
  Centavos monto = 0;     // siempre positivo; el tipo define el sentido
  std::string nota;
  // Si el movimiento nació de una plantilla de reparto o de un recurrente,
  // aquí queda de cuál, para poder rastrearlo.
  std::string origenAutomatico;
};

// ---------------------------------------------------------------------------
// Pasivo
// ---------------------------------------------------------------------------

// Lo que se debe. El caso que motiva la aplicación es la tarjeta de crédito:
// parte del saldo de la cuenta ya le pertenece a ella.
struct Pasivo {
  Id id = kSinId;
  std::string nombre;
  Centavos saldo = 0;  // positivo significa deuda
  std::string nota;
};

// ---------------------------------------------------------------------------
// Periodicidad
// ---------------------------------------------------------------------------

enum class Periodicidad {
  Semanal = 0,
  Quincenal = 1,
  Mensual = 2,
};

std::string nombreDePeriodicidad(Periodicidad p);
// Cuántos periodos de este tipo hay en un año: 52, 24 y 12.
int periodosPorAnio(Periodicidad p);

// ---------------------------------------------------------------------------
// Meta
// ---------------------------------------------------------------------------

struct Meta {
  Id id = kSinId;
  std::string nombre;
  Centavos montoObjetivo = 0;
  Fecha fechaObjetivo;
  Id sobreAsociado = kSinId;
  Centavos aportePlaneado = 0;  // por cada periodo
  Periodicidad periodicidad = Periodicidad::Mensual;
  // Tasa efectiva anual esperada, como fracción: 0.085 es 8.5% anual.
  double tasaAnual = 0.0;
  std::string nota;
};

// ---------------------------------------------------------------------------
// Recurrente
// ---------------------------------------------------------------------------

// Plantilla de movimiento que se repite. No se materializa sola: el usuario
// aprieta un botón cuando el movimiento realmente ocurrió.
struct Recurrente {
  Id id = kSinId;
  std::string nombre;
  TipoMovimiento tipo = TipoMovimiento::Gasto;
  Id sobreOrigen = kSinId;
  Id sobreDestino = kSinId;
  Id categoria = kSinId;
  Centavos monto = 0;
  std::string nota;
  Periodicidad periodicidad = Periodicidad::Mensual;
  Fecha proximaFecha;
  bool activo = true;
};

// ---------------------------------------------------------------------------
// Plantilla de reparto
// ---------------------------------------------------------------------------

enum class ModoReparto {
  // Cada línea lleva un monto fijo en centavos.
  MontosFijos = 0,
  // Cada línea lleva un porcentaje del total que se está repartiendo.
  Porcentajes = 1,
};

struct LineaReparto {
  Id sobre = kSinId;
  // En modo MontosFijos, centavos. En modo Porcentajes, puntos base:
  // 10000 puntos base son 100%, 2550 son 25.5%.
  std::int64_t valor = 0;
  // La línea que absorbe el centavo sobrante del redondeo. Si ninguna está
  // marcada, lo absorbe la línea de mayor valor.
  bool recibeResto = false;
};

struct PlantillaReparto {
  Id id = kSinId;
  std::string nombre;
  ModoReparto modo = ModoReparto::Porcentajes;
  // Para modo Porcentajes: el monto que se sugiere repartir (por ejemplo, el
  // sueldo quincenal). Es solo un valor por omisión, se puede cambiar al
  // aplicar.
  Centavos montoSugerido = 0;
  // De dónde sale el dinero. Si es kSinId, el reparto genera entradas: el
  // dinero llega de fuera y se distribuye. Si apunta a un sobre, genera
  // traspasos desde ese sobre.
  Id sobreFuente = kSinId;
  Id categoria = kSinId;  // opcional, se copia a los movimientos generados
  std::vector<LineaReparto> lineas;
};

}  // namespace sobres
