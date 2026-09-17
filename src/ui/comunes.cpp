#include "comunes.h"

#include <QCompleter>
#include <QHeaderView>
#include <QStringListModel>
#include <QVariant>

namespace sobres {

const std::vector<QString>& coloresSugeridos() {
  static const std::vector<QString> colores = {
      QStringLiteral("#4b6bfb"), QStringLiteral("#0f766e"),
      QStringLiteral("#b45309"), QStringLiteral("#9d174d"),
      QStringLiteral("#4d7c0f"), QStringLiteral("#6d28d9"),
      QStringLiteral("#0369a1"), QStringLiteral("#a16207"),
      QStringLiteral("#be123c"), QStringLiteral("#475569")};
  return colores;
}

CampoMonto::CampoMonto(QWidget* padre) : QLineEdit(padre) {
  setPlaceholderText(QStringLiteral("0.00"));
  setAlignment(Qt::AlignRight);
  setMaxLength(20);
}

void CampoMonto::fijarMonto(Centavos monto) {
  setText(QString::fromStdString(formatearPesos(monto, false)));
}

bool CampoMonto::estaVacio() const { return text().trimmed().isEmpty(); }

std::optional<Centavos> CampoMonto::monto() const {
  const auto valor = leerPesos(text().toStdString());
  return valor;
}

CampoCategoria::CampoCategoria(QWidget* padre) : QComboBox(padre) {
  setEditable(true);
  // Que no meta sola en la lista lo que se teclea: el alta la decide la
  // aplicación al guardar, no el widget.
  setInsertPolicy(QComboBox::NoInsert);
  setPlaceholderText(QStringLiteral("Escribe una categoría"));
  lineEdit()->setPlaceholderText(
      QObject::tr("Escribe una categoría; si es nueva se crea sola"));

  auto* asistente = new QCompleter(this);
  asistente->setCaseSensitivity(Qt::CaseInsensitive);
  // Sugiere por cualquier parte del nombre, no solo por el principio: al
  // escribir "super" aparece "Súper y despensa".
  asistente->setFilterMode(Qt::MatchContains);
  asistente->setCompletionMode(QCompleter::PopupCompletion);
  setCompleter(asistente);
}

void CampoCategoria::fijarCategorias(const std::vector<Categoria>& categorias) {
  const QString escrito = currentText();
  clear();
  QStringList nombres;
  for (const Categoria& c : categorias) {
    const QString nombre = QString::fromStdString(c.nombre);
    addItem(nombre, QVariant(static_cast<qlonglong>(c.id)));
    nombres << nombre;
  }
  if (completer() != nullptr) {
    auto* modelo = new QStringListModel(nombres, completer());
    completer()->setModel(modelo);
  }
  setCurrentText(escrito);
}

void CampoCategoria::fijarSeleccion(Id id,
                                    const std::vector<Categoria>& categorias) {
  for (const Categoria& c : categorias) {
    if (c.id == id) {
      setCurrentText(QString::fromStdString(c.nombre));
      return;
    }
  }
  setCurrentText(QString());
}

QString CampoCategoria::nombreCapturado() const {
  return currentText().trimmed();
}

void llenarConSobres(QComboBox* combo, const std::vector<Sobre>& sobres,
                     bool incluirNinguno, const QString& textoNinguno) {
  const Id anterior = idSeleccionado(combo);
  combo->clear();
  if (incluirNinguno) {
    combo->addItem(textoNinguno.isEmpty() ? QStringLiteral("—") : textoNinguno,
                   QVariant(static_cast<qlonglong>(kSinId)));
  }
  for (const Sobre& s : sobres) {
    QString etiqueta = QString::fromStdString(s.nombre);
    if (s.tipo == TipoSobre::Ajeno) {
      etiqueta += QStringLiteral("  (de terceros)");
    }
    combo->addItem(etiqueta, QVariant(static_cast<qlonglong>(s.id)));
  }
  seleccionarId(combo, anterior);
}

void llenarConCategorias(QComboBox* combo,
                         const std::vector<Categoria>& categorias,
                         bool incluirNinguna) {
  const Id anterior = idSeleccionado(combo);
  combo->clear();
  if (incluirNinguna) {
    combo->addItem(QStringLiteral("Sin categoría"),
                   QVariant(static_cast<qlonglong>(kSinId)));
  }
  for (const Categoria& c : categorias) {
    QString etiqueta = QString::fromStdString(c.nombre);
    if (!c.icono.empty()) {
      etiqueta = QString::fromStdString(c.icono) + QStringLiteral("  ") + etiqueta;
    }
    combo->addItem(etiqueta, QVariant(static_cast<qlonglong>(c.id)));
  }
  seleccionarId(combo, anterior);
}

Id idSeleccionado(const QComboBox* combo) {
  if (combo->currentIndex() < 0) return kSinId;
  return static_cast<Id>(combo->currentData().toLongLong());
}

void ajustarAlturaDeTabla(QTableWidget* tabla, int filasVisiblesMaximas,
                          int filasVisiblesMinimas) {
  const int alturaFila = tabla->rowCount() > 0
                             ? tabla->rowHeight(0)
                             : tabla->verticalHeader()->defaultSectionSize();
  int filas = tabla->rowCount();
  if (filas < filasVisiblesMinimas) filas = filasVisiblesMinimas;
  if (filas > filasVisiblesMaximas) filas = filasVisiblesMaximas;
  const int alto = tabla->horizontalHeader()->height() + filas * alturaFila + 6;
  tabla->setMinimumHeight(alto);
  tabla->setMaximumHeight(alto);
}

void seleccionarId(QComboBox* combo, Id id) {
  for (int k = 0; k < combo->count(); ++k) {
    if (static_cast<Id>(combo->itemData(k).toLongLong()) == id) {
      combo->setCurrentIndex(k);
      return;
    }
  }
  if (combo->count() > 0) combo->setCurrentIndex(0);
}

}  // namespace sobres
