// Sobres — comunes.h
// Piezas de interfaz que se repiten en varias pantallas: el campo donde se
// escribe un monto, los selectores de sobre y categoría, y las conversiones
// entre las fechas del núcleo y las de Qt.
#pragma once

#include <QComboBox>
#include <QDate>
#include <QLineEdit>
#include <QString>
#include <QTableWidget>
#include <vector>

#include "../nucleo/modelos.h"

namespace sobres {

class Estado;

// --- Conversiones ---------------------------------------------------------

inline QDate aQDate(const Fecha& f) { return QDate(f.anio, f.mes, f.dia); }
inline Fecha deQDate(const QDate& d) {
  return Fecha{d.year(), d.month(), d.day()};
}
inline QString pesos(Centavos monto, bool conSigno = false) {
  return QString::fromStdString(formatearPesos(monto, true, conSigno));
}

// --- Paleta ---------------------------------------------------------------

namespace paleta {
inline constexpr const char* kTinta = "#1f2933";
inline constexpr const char* kTintaSuave = "#6b7280";
inline constexpr const char* kLinea = "#dfe3e8";
inline constexpr const char* kFondo = "#f7f7f5";
inline constexpr const char* kPapel = "#ffffff";
inline constexpr const char* kPositivo = "#15803d";
inline constexpr const char* kNegativo = "#b91c1c";
}  // namespace paleta

// Colores sugeridos al crear un sobre o una categoría, para no dejar al usuario
// frente a un selector de color en blanco.
const std::vector<QString>& coloresSugeridos();

// --- Campo de monto -------------------------------------------------------

// Un campo de texto que solo acepta montos y los entrega en centavos. Se
// escribe "1,234.56" o "1234.56" y sale 123456.
class CampoMonto : public QLineEdit {
  Q_OBJECT

 public:
  explicit CampoMonto(QWidget* padre = nullptr);

  void fijarMonto(Centavos monto);
  // Devuelve el monto capturado, o nada si el texto no es un monto válido.
  std::optional<Centavos> monto() const;
  bool estaVacio() const;
};

// --- Campo de categoría ---------------------------------------------------

// Campo donde se escribe la categoría. Mientras se teclea va sugiriendo las
// que ya existen, y si el nombre es nuevo la categoría se da de alta al
// guardar el movimiento. La idea es no obligar a nadie a ir primero a Ajustes
// a crear la categoría y luego volver a capturar el gasto.
class CampoCategoria : public QComboBox {
  Q_OBJECT

 public:
  explicit CampoCategoria(QWidget* padre = nullptr);

  void fijarCategorias(const std::vector<Categoria>& categorias);
  // Deja escrito el nombre de esta categoría, si sigue existiendo.
  void fijarSeleccion(Id id, const std::vector<Categoria>& categorias);
  // El nombre capturado, sin espacios sobrantes.
  QString nombreCapturado() const;
};

// --- Selectores -----------------------------------------------------------

// Llena un combo con los sobres activos. Si incluirNinguno, agrega una primera
// opción vacía que devuelve kSinId.
void llenarConSobres(QComboBox* combo, const std::vector<Sobre>& sobres,
                     bool incluirNinguno = false,
                     const QString& textoNinguno = QString());
void llenarConCategorias(QComboBox* combo,
                         const std::vector<Categoria>& categorias,
                         bool incluirNinguna = true);

Id idSeleccionado(const QComboBox* combo);
void seleccionarId(QComboBox* combo, Id id);

// --- Tablas ---------------------------------------------------------------

// Ajusta la altura de una tabla al número de filas que tiene, hasta un tope,
// para que una tabla con dos renglones no ocupe media pantalla en blanco.
void ajustarAlturaDeTabla(QTableWidget* tabla, int filasVisiblesMaximas = 8,
                          int filasVisiblesMinimas = 3);

}  // namespace sobres
