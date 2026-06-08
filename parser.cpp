// Il file è fatto appositamente per leggere file .omap che non sono altro che
// degli XML che hanno tutta una parte precedente sui simboli. Ogni file usa
// numeri diversi per chiamare lo stesso simbolo ma all'inizio dei file vengono
// chiamati tutti i simboli e c'è una corrispondenza individuabile tra i numeri
// utilizzati in quel file e gli id generici del mondo dell'orienteering. Dunque
// il programma crea un corrispettivo tra questi due numeri.

// C'è un vettore di id\_generici vietati, grazie al quale si possono ottenere i
// numeri specifici utilizzati dal file per gli oggetti vietati. Viene
// utilizzato lo stesso metodo per gli oggetti pieni.

// Tra i simboli dell'orienteering c'è un simbolo che è diventato oltrepassabile
// da pochi anni quindi può capitare che sia considerato come non oltrepassabile
// in alcune mappe vecchie, perciò nel programma è lasciata la possibilità di
// scelta all'utente.

// Una volta trovati i numeri si possono cercare gli "object"(solo quelli con id
// vietati) nel file, e si salvano le coordinate. Una volta trovati tutti gli
// oggetti vietati si trovano il massimo e il minimo in x e in y e si calcolano
// quindi i limiti della griglia che viene creata con risoluzione fissa di 800.
// La risoluzione corrisponde al numero di bool della larghezza mentre la
// lunghezza viene calcolata in base alle proporzione dei massimi-minimi in x e
// y.

// La griglia parte libera e prima vengono segnati i bordi usando l'algoritmo di
// Bresenham, o le curve di Bezier poi, se il simbolo è pieno, viene riempito
// orizzontalmente: per ogni riga si parte dal margine e la riga si riempie dopo
// aver attraversato un numero dispari di volte il bordo fino a che non si
// incontra il bordo successivo di quell'oggetto. Si ottiene così la griglia di
// bool che viene stampata in CSV per essere vista meglio. La griglia, la
// posizione di partenza e arrivo passano al pathfinding  che restituirà i
// percorsi.

// Si crea un file .omap uguale all'originale a cui vengono aggiunti i percorsi.

#include "parser.hpp"
#include "path.hpp"
#include <algorithm>
#include <cassert>
#include <cmath>
#include <cstddef>
#include <fstream>
#include <iostream>
#include <string>
#include <unordered_set>
#include <vector>
namespace pf {
std::vector<Path> // funzione wrapper
final_aggiratore(const Point &A, const Point &B,
                 const std::vector<std::vector<bool>> &griglia) {
  assert(!griglia.empty());
  assert(!griglia[0].empty());
  Grid grid(griglia);
  grid.controlla_id_ostacolo();
  std::vector<Path> allPaths = trova_paths(A, B, grid);
  std::vector<Path> paths = filtra_paths(allPaths);
  return paths;
}

size_t salta_virgolette_spazi(const std::string &linea, size_t inizio) {
  while (inizio < linea.size() &&
         (linea[inizio] == '"' || linea[inizio] == ' ')) {
    ++inizio;
  }
  return inizio;
}

// data una stringa: linea, se rappresenta un simbolo, lo trova. Trova tutti i
// simboli presenti sulla mappa
int trova_simbolo(const std::string &linea) {
  size_t inizio_del_numero_id{0};
  if (linea.find("symbol=") != std::string::npos) {
    inizio_del_numero_id =
        salta_virgolette_spazi(linea, 7 + linea.find("symbol="));
  } else {
    return -1;
  }
  if (isdigit(linea[inizio_del_numero_id])) {
    std::string numero;
    while (inizio_del_numero_id < linea.size() &&
           isdigit(linea[inizio_del_numero_id])) {
      numero += linea[inizio_del_numero_id];
      ++inizio_del_numero_id;
    }
    return stoi(numero);
  } else {
    return -1;
  }
}

// per la stringa: linea, se rappresenta un simbolo, trova id corrispondente, si
// crea così il vettore di simboli-id
id_to_symbol ids_to_symbols(const std::string &linea) {
  size_t inizio_id{0};
  size_t inizio_symbol{0};
  if (linea.find("id=") != std::string::npos &&
      linea.find("code=") != std::string::npos) {
    inizio_id = salta_virgolette_spazi(linea, 3 + linea.find("id="));
    inizio_symbol = salta_virgolette_spazi(linea, 5 + linea.find("code="));
  } else {
    return {-1, -1};
  }
  std::string numero_id;
  std::string numero_symbol;
  while (inizio_id < linea.size() && isdigit(linea[inizio_id])) {
    numero_id += linea[inizio_id];
    ++inizio_id;
  }
  while (inizio_symbol < linea.size() && isdigit(linea[inizio_symbol])) {
    numero_symbol += linea[inizio_symbol];
    ++inizio_symbol;
  }
  if (numero_id.empty() || numero_symbol.empty()) {
    return {-1, -1};
  }
  return id_to_symbol{std::stoi(numero_id), std::stoi(numero_symbol)};
}

// da simboli vietati/pieni(iof), trova id vietati/pieni, utilizzando il vettore
// ottenuto dai vari id_to_symbol
std::vector<int>
trova_id_vietati_pieni(const std::vector<id_to_symbol> &ids_symbols,
                       const std::vector<int> &vietati_o_pieni) {
  std::vector<int> risultato{};
  for (const auto &coppia : ids_symbols) {
    for (int simbolo_vietato : vietati_o_pieni) {
      if (coppia.symbol == simbolo_vietato) {
        risultato.push_back(coppia.id);
      }
    }
  }
  return risultato;
}

double calcola_fattore_scala() {
  std::cout << "Inserire scala della mappa (se è 1:4000, scrivere 4000)\n";
  double scala = 0;
  if (!(std::cin >> scala)) {
    std::cout << "Input non valido\n";
    return 1.0;
  }
  assert(scala > 0);
  return scala / 1000000;
}

// Funzione per trovare tutte le coordinate================================
std::vector<Point> estrai_coordinate(const std::string &linea,
                                     const double fattore_scala) {
  std::vector<Point> punti{};
  std::string stringa_coordinate{};
  auto inizio = linea.find("<coords");
  if (inizio != std::string::npos) {
    inizio = linea.find(">", inizio) + 1;
    auto fine =
        linea.find("</coords>", inizio); // nelle righe di coordinate di punti
    if (fine != std::string::npos) {
      stringa_coordinate = linea.substr(inizio, fine - inizio);
    }
  }
  if (stringa_coordinate.empty()) {
    return punti;
  }

  // token
  std::vector<std::string> token;
  size_t pos = 0;
  while (pos < stringa_coordinate.size()) {
    size_t end = stringa_coordinate.find(';', pos);
    if (end == std::string::npos) {
      end = stringa_coordinate.size();
    }
    std::string pezzo = stringa_coordinate.substr(pos, end - pos);
    // rimuove spazi iniziali/finali
    size_t primo = pezzo.find_first_not_of(" \t");
    if (primo != std::string::npos) {
      size_t ultimo = pezzo.find_last_not_of(" \t");
      pezzo = pezzo.substr(primo, ultimo - primo + 1);
      if (!pezzo.empty()) {
        token.push_back(pezzo);
      }
    }
    pos = end + 1;
  }

  // struct punto+flag
  struct PuntoFlag {
    double x, y;
    int flag;
  };

  std::vector<PuntoFlag> punti_flag;
  for (const auto &t : token) {
    std::vector<std::string> parti;
    size_t start = 0;
    while (start < t.size()) {
      size_t sp = t.find(' ', start);
      if (sp == std::string::npos) {
        sp = t.size();
      }
      std::string part = t.substr(start, sp - start);
      if (!part.empty()) {
        parti.push_back(part);
      }
      start = sp + 1;
    }
    if (parti.size() >= 2) {
      double x = std::stod(parti[0]);
      double y = std::stod(parti[1]);
      int flag = 0;
      if (parti.size() >= 3) {
        flag = std::stoi(parti[2]);
      }
      punti_flag.push_back({x, y, flag});
    }
  }

  if (punti_flag.empty()) {
    return punti;
  }

  // applica fattore di scala
  for (auto &pf : punti_flag) {
    pf.x *= fattore_scala;
    pf.y *= fattore_scala;
  }

  // quanti nodi di curva ci sono
  std::vector<size_t> indici_nodi;
  for (size_t i = 0; i < punti_flag.size(); ++i) {
    if (punti_flag[i].flag == 1) {
      indici_nodi.push_back(i);
      i += 2; // salta i due punti di "controllo"
      continue;
    } else {
      indici_nodi.push_back(i);
    }
  }
  // se 0, normale
  if (indici_nodi.empty()) {
    for (const auto &pf : punti_flag) {
      punti.push_back({static_cast<int>(pf.x), static_cast<int>(pf.y)});
    }
    return punti;
  }

  // formula bezier cubica
  auto bezier_cubica = [](const PuntoFlag &p0, const PuntoFlag &p1,
                          const PuntoFlag &p2, const PuntoFlag &p3,
                          double t) -> PuntoFlag {
    double u = 1.0 - t;
    double x = u * u * u * p0.x + 3.0 * u * u * t * p1.x +
               3.0 * u * t * t * p2.x + t * t * t * p3.x;
    double y = u * u * u * p0.y + 3.0 * u * u * t * p1.y +
               3.0 * u * t * t * p2.y + t * t * t * p3.y;
    return {x, y, 0};
  };

  // divide in n =10 segmentini la curva tra due nodi
  auto aggiungi_segmento = [&](size_t inizio_idx, size_t fine_idx) {
    const auto &nodo_inizio = punti_flag[inizio_idx];
    const auto &nodo_fine = punti_flag[fine_idx];
    int num_nodi_di_curva = static_cast<int>(fine_idx - inizio_idx - 1);

    if (num_nodi_di_curva == 2) {
      // Curva cubica
      const auto &nodo_di_curva_1 = punti_flag[inizio_idx + 1];
      const auto &nodo_di_curva_2 = punti_flag[inizio_idx + 2];
      const int passi = 10; // messo 10 segmentini
      for (int s = 1; s <= passi; ++s) {
        double t = static_cast<double>(s) / passi;
        auto pt = bezier_cubica(nodo_inizio, nodo_di_curva_1, nodo_di_curva_2,
                                nodo_fine, t);
        punti.push_back({static_cast<int>(pt.x), static_cast<int>(pt.y)});
      }
    } else if (num_nodi_di_curva == 1) {
      // Curva quadraticas
      const auto &c = punti_flag[inizio_idx + 1];
      const int passi = 10;
      for (int s = 1; s <= passi; ++s) {
        double t = static_cast<double>(s) / passi;
        double u = 1.0 - t;
        double x =
            u * u * nodo_inizio.x + 2 * u * t * c.x + t * t * nodo_fine.x;
        double y =
            u * u * nodo_inizio.y + 2 * u * t * c.y + t * t * nodo_fine.y;
        punti.push_back({static_cast<int>(x), static_cast<int>(y)});
      }
    } else {
      // se problemi fa linea retta
      for (size_t i = inizio_idx + 1; i <= fine_idx; ++i) {
        punti.push_back({static_cast<int>(punti_flag[i].x),
                         static_cast<int>(punti_flag[i].y)});
      }
    }
  };

  // aggiunge primo nodo
  punti.push_back({static_cast<int>(punti_flag[indici_nodi[0]].x),
                   static_cast<int>(punti_flag[indici_nodi[0]].y)});

  // e segmenti
  for (size_t k = 0; k < indici_nodi.size() - 1; ++k) {
    aggiungi_segmento(indici_nodi[k], indici_nodi[k + 1]);
  }

  return punti;
}

void calcola_dimensioni_max_min(
    const std::vector<oggetto_poligonale_vietato> &oggetti, double &min_x,
    double &max_x, double &min_y, double &max_y, const Point &arrivo,
    const Point &partenza) {

  min_x = std::min(arrivo.x, partenza.x);
  max_x = std::max(arrivo.x, partenza.x);
  min_y = std::min(arrivo.y, partenza.y);
  max_y = std::max(arrivo.y, partenza.y);

  for (const auto &oggetto : oggetti) {
    for (const auto &punto : oggetto.contorno) {
      min_x = std::min(min_x, static_cast<double>(punto.x));
      max_x = std::max(max_x, static_cast<double>(punto.x));
      min_y = std::min(min_y, static_cast<double>(punto.y));
      max_y = std::max(max_y, static_cast<double>(punto.y));
    }
  }

  double larghezza = max_x - min_x;
  double altezza = max_y - min_y;
  min_x -= larghezza * 0.05;
  max_x += larghezza * 0.05;
  min_y -= altezza * 0.05;
  max_y += altezza * 0.05;

  assert(max_x >= min_x);
  assert(max_y >= min_y);
}

// converte coordinate del mondo a griglia
void mondo_a_griglia(double x, double y, double min_x, double max_x,
                     double min_y, double max_y, int larghezza_griglia,
                     int altezza_griglia, int &gx, int &gy) {
  double larghezza_mondo = max_x - min_x;
  double altezza_mondo = max_y - min_y;

  gx = static_cast<int>((x - min_x) / larghezza_mondo * larghezza_griglia);
  gy = static_cast<int>((max_y - y) / altezza_mondo * altezza_griglia);

  if (gx < 0) {
    gx = 0;
  }
  if (gx >= larghezza_griglia) {
    gx = larghezza_griglia - 1;
  }
  if (gy < 0) {
    gy = 0;
  }
  if (gy >= altezza_griglia) {
    gy = altezza_griglia - 1;
  }

  assert(gx >= 0 && gx < larghezza_griglia);
  assert(gy >= 0 && gy < altezza_griglia);
}

// trova intersezioni rispetto a retta orizzontale
bool calcola_intersezione(int x1, int y1, int x2, int y2, int y, int &x) {
  if (y2 != y1) {
    if ((y2 < y && y1 >= y) || (y2 >= y && y1 < y)) {
      x = x1 + (y - y1) * (x2 - x1) / (y2 - y1);
      return true;
    }
  }
  return false;
}

// per ogni poligono, per ogni riga, dopo che da un lato numero di intersezioni
// è dispari, riempie ====> controllo se è pieno in oggetti_to_griglia
void riempi_poligoni(std::vector<std::vector<bool>> &griglia,
                     const std::vector<std::pair<int, int>> &poligono,
                     int larghezza_griglia, int altezza_griglia) {
  assert(larghezza_griglia > 0);
  assert(altezza_griglia > 0);
  assert(!poligono.empty());

  int min_x = poligono[0].first;
  int max_x = poligono[0].first;
  int min_y = poligono[0].second;
  int max_y = poligono[0].second;
  for (const auto &punto : poligono) {
    if (punto.first < min_x) {
      min_x = punto.first;
    } else if (punto.first > max_x) {
      max_x = punto.first;
    }
    if (punto.second < min_y) {
      min_y = punto.second;
    } else if (punto.second > max_y) {
      max_y = punto.second;
    }
  }

  for (int y = min_y; y <= max_y; ++y) {
    std::vector<int> intersezioni{};
    size_t n_vertici = poligono.size();
    for (size_t i = 0; i < n_vertici; ++i) {
      const auto &p1 = poligono[i];
      const auto &p2 = poligono[(i + 1) % n_vertici];
      int intersezione_x;
      if (calcola_intersezione(p1.first, p1.second, p2.first, p2.second, y,
                               intersezione_x)) {
        intersezioni.push_back(intersezione_x);
      }
    }
    if (!intersezioni.empty()) {
      std::sort(intersezioni.begin(), intersezioni.end());
      for (size_t i = 0; i + 1 < intersezioni.size(); i += 2) {
        for (int x = intersezioni[i]; x <= intersezioni[i + 1]; ++x) {
          if (x >= 0 && x < larghezza_griglia && y >= 0 &&
              y < altezza_griglia) {
            griglia[static_cast<size_t>(y)][static_cast<size_t>(x)] = false;
          }
        }
      }
    }
  }
}

// user decide se verde 3(per anni è stato non oltrepassabile, ora è
// oltrepassabile)
void verde3_oltrepassabile(std::vector<int> &ids_vietati,
                           std::vector<int> &ids_pieni) {
  std::cout << "Il verde 3 è oltrepassabile? (y/n)\n";
  std::string risposta;
  std::cin >> risposta;
  if (risposta != "Y" && risposta != "y" && risposta != "yes" &&
      risposta != "Yes" && risposta != "S" && risposta != "s" &&
      risposta != "sì" && risposta != "si" && risposta != "Sì" &&
      risposta != "Si") {
    ids_vietati.push_back(410);
    ids_pieni.push_back(410);
  }
}

// per ogni oggetto lo disegna sulla griglia, linee con bresenham
void oggetti_to_griglia(const oggetto_poligonale_vietato &oggetto,
                        std::vector<std::vector<bool>> &griglia, double min_x,
                        double max_x, double min_y, double max_y,
                        int larghezza_griglia, int altezza_griglia) {

  assert(larghezza_griglia > 0);
  assert(altezza_griglia > 0);

  if (oggetto.contorno.size() < 2) {
    return;
  }

  auto disegna_linea = [&](int x1, int y1, int x2,
                           int y2) { // lambda per disegnare linee
    int dx = abs(x2 - x1);
    int dy = abs(y2 - y1);
    int sx = (x1 < x2) ? 1 : -1;
    int sy = (y1 < y2) ? 1 : -1;
    int err = dx - dy;

    int x = x1;
    int y = y1;

    while (true) {
      if (x >= 0 && x < larghezza_griglia && y >= 0 && y < altezza_griglia) {
        griglia[static_cast<size_t>(y)][static_cast<size_t>(x)] = false;
      }

      if (x == x2 && y == y2) {
        break;
      }

      int e2 = 2 * err;
      if (e2 > -dy) {
        err -= dy;
        x += sx;
      }
      if (e2 < dx) {
        err += dx;
        y += sy;
      }
    }
  };

  std::vector<std::pair<int, int>> punti_griglia;
  punti_griglia.reserve(oggetto.contorno.size());

  for (const auto &punto : oggetto.contorno) {
    int gx{0};
    int gy{0};
    // trasforma coordinate
    mondo_a_griglia(static_cast<double>(punto.x), static_cast<double>(punto.y),
                    min_x, max_x, min_y, max_y, larghezza_griglia,
                    altezza_griglia, gx, gy);
    punti_griglia.push_back({gx, gy});
  }

  for (size_t i = 0; i < punti_griglia.size() - 1; ++i) { // disegna
    disegna_linea(punti_griglia[i].first, punti_griglia[i].second,
                  punti_griglia[i + 1].first, punti_griglia[i + 1].second);
  }
  // se è pieno riempie
  if (oggetto.e_pieno && punti_griglia.size() > 2) {
    riempi_poligoni(griglia, punti_griglia, larghezza_griglia, altezza_griglia);
  }
}

// aumenta di spessore di raggio 1 tutto per evitare gli attraversamenti in
// diagonale delle linee di spessore 1
void dilata_ostacoli(std::vector<std::vector<bool>> &griglia) {
  const int altezza = static_cast<int>(griglia.size());
  if (altezza == 0) {
    return;
  }

  const int larghezza = static_cast<int>(griglia[0].size());
  assert(larghezza > 0);

  auto nuova_griglia = griglia;

  for (int y = 0; y < altezza; ++y) {
    for (int x = 0; x < larghezza; ++x) {
      if (!griglia[static_cast<size_t>(y)]
                  [static_cast<size_t>(x)]) // cella vietata
      {
        for (int dy = -1; dy <= 1; ++dy) {
          for (int dx = -1; dx <= 1; ++dx) {
            int nx = x + dx;
            int ny = y + dy;

            if (nx >= 0 && nx < larghezza && ny >= 0 && ny < altezza) {
              nuova_griglia[static_cast<size_t>(ny)][static_cast<size_t>(nx)] =
                  false;
            }
          }
        }
      }
    }
  }

  griglia = std::move(nuova_griglia);
}

double costo_path_geometrico_mondo(
    const std::vector<Point>
        &path, // per calcolare il costo effettivo serve ritrasformare le
               // coordinate in quelle del mondo e poi calcolare
    double min_x, double max_x, double min_y, double max_y,
    int larghezza_griglia, int altezza_griglia, double fattore_scala) {
  if (path.size() < 2)
    return 0.0;

  auto to_world = [&](const Point &p) {
    double x = min_x + (p.x + 0.5) * (max_x - min_x) / larghezza_griglia;
    double y = max_y - (p.y + 0.5) * (max_y - min_y) / altezza_griglia;

    return std::pair<double, double>{x / fattore_scala, y / fattore_scala};
  };

  double cost = 0.0;

  auto prev = to_world(path[0]);

  for (size_t i = 1; i < path.size(); ++i) {
    auto cur = to_world(path[i]);

    double dx = cur.first - prev.first;
    double dy = cur.second - prev.second;

    cost += std::hypot(dx, dy);

    prev = cur;
  }

  return cost * 4 / 1000;
}

// esporta in un altro file omap
void esporta_percorso_omap(const std::string &filename_input,
                           const std::string &filename_output,
                           const std::vector<Path> &paths,
                           const std::vector<id_to_symbol> &ids_symbols,
                           double min_x, double max_x, double min_y,
                           double max_y, int larghezza_griglia,
                           int altezza_griglia, double fattore_scala) {

  assert(!paths.empty());
  assert(!ids_symbols.empty());

  std::ifstream in(filename_input);   // input
  std::ofstream out(filename_output); // output

  if (!in.is_open() || !out.is_open()) {
    std::cerr << "Errore apertura file\n";
    return;
  }

  int id_linea_disegno{0};
  for (const auto &coppia : ids_symbols) {
    if (coppia.symbol == 704) {
      id_linea_disegno = coppia.id;
      break;
    }
  }

  std::string linea;

  bool oggetti_aggiunti = false;

  // per ogni linea
  while (std::getline(in, linea)) {
    // se trova </objects> che è quello alla fine degli oggetti di solito
    // inserisce i percorsi
    if (!oggetti_aggiunti &&
        linea.find("</objects>") !=
            std::string::npos) { // trova dove c'è l'ultimo oggetto
      out << "Qui iniziano i percorsi\n";
      for (const auto &path : paths) {
        auto percorso = path.points;
        out << "      <object type=\"1\" symbol=\""
            << id_linea_disegno // inserisce i percorsi
            << "\">";
        out << "        <coords count=\"" << percorso.size() << "\">";

        for (size_t i = 0; i < percorso.size(); ++i) {
          double x_mondo = min_x + (percorso[i].x + 0.5) * (max_x - min_x) /
                                       larghezza_griglia;
          double y_mondo =
              max_y - (percorso[i].y + 0.5) * (max_y - min_y) / altezza_griglia;

          int x_f = static_cast<int>(x_mondo / fattore_scala);
          int y_f = static_cast<int>(y_mondo / fattore_scala);

          out << (x_f) << " " << (y_f);
          if (i < (percorso.size() - 1)) {
            out << ";";
          } else {
            out << " 16;"; // flag finale delle coordinate
          }
        }

        out << "</coords>";
        out << "<pattern rotation=\"0\"><coord x=\"0\" "
               "y=\"0\"/></pattern>";
        out << "</object>\n";
        out << "costo del percorso precedente:" << path.cost
            << "m\n"; // conversione lunghezza percorsi
      }
      out << "Qui finiscono i percorsi";
      oggetti_aggiunti = true;
    }
    // altrimenti copia la linea
    out << linea << "\n"; // copia le linee
  }

  in.close();
  out.close(); // chiude i file
  std::cout << "File OMAP con percorso salvato: " << filename_output << "\n";
}

// tutto il main ma non nel main(per test su file omap)
std::vector<Path> helper_main(const std::string &nome_file) {

  std::vector<int> vietati{206, 301, 307, 411, 515, 518,
                           520, 521, 708, 709, 714}; // simboli generali vietati
  std::vector<int> pieni{206, 301, 307, 411,
                         520, 521, 709, 714}; // simboli generali pieni
  int id_arrivo{0};
  int id_partenza{0};
  bool in_undo_redo = false;
  std::unordered_set<int> id_vietati{};
  std::unordered_set<int> id_pieni{};
  std::vector<id_to_symbol> ids_symbols{};
  Point partenza_mondo;
  Point arrivo_mondo;
  bool partenza_trovata = false;
  bool arrivo_trovato = false;
  std::string linea;
  std::ifstream file(nome_file);
  if (!file.is_open()) {
    std::cout << "File non trovato\n";
    return {};
  }
  double fattore_scala = calcola_fattore_scala();
  std::vector<oggetto_poligonale_vietato> oggetti_vietati{};
  while (std::getline(file, linea)) {
    if (linea.find("id=") != std::string::npos &&
        linea.find("<symbol") != std::string::npos) {
      id_to_symbol coppia = ids_to_symbols(linea);
      if (coppia.id != -1 && coppia.symbol != -1) {
        ids_symbols.push_back(coppia);
      }
    }
  }
  assert(!ids_symbols.empty()); // almeno un simbolo

  verde3_oltrepassabile(vietati, pieni);
  std::vector<int> vettore_ids_vietati =
      trova_id_vietati_pieni(ids_symbols, vietati);
  std::vector<int> vettore_ids_pieni =
      trova_id_vietati_pieni(ids_symbols, pieni);
  id_vietati = {vettore_ids_vietati.begin(), vettore_ids_vietati.end()};
  id_pieni = {vettore_ids_pieni.begin(), vettore_ids_pieni.end()};
  for (const auto &coppia : ids_symbols) {
    if (coppia.symbol == 706) {
      id_arrivo = coppia.id;
    } else if (coppia.symbol == 701) {
      id_partenza = coppia.id;
    }
  }
  // ho trovato tutte associazioni id-simbolo, rileggo il file cercando gli
  // oggetti

  file.clear();
  file.seekg(0);
  while (std::getline(
      file, linea)) { // serve per non leggere come oggetti quelli degli undo
    if ((linea.find("<undo>") != std::string::npos) ||
        (linea.find("<redo>") != std::string::npos)) {
      in_undo_redo = true;
      continue;
    }
    if ((linea.find("</undo>") != std::string::npos) ||
        (linea.find("</redo>") != std::string::npos)) {
      in_undo_redo = false;
      continue;
    }
    if (in_undo_redo) {
      continue;
    }
    if (linea.find("<object") != std::string::npos) {
      std::string blocco = linea;

      // Se l'oggetto non è tutto su una riga, accumula
      while (blocco.find("</object>") == std::string::npos &&
             blocco.find("/>") == std::string::npos) {
        std::string riga_extra;
        if (!std::getline(file, riga_extra)) {
          break;
        }
        blocco += " " + riga_extra;
      }
      int id = trova_simbolo(blocco);
      if (id == id_partenza) {
        std::vector<Point> coordinate =
            estrai_coordinate(blocco, fattore_scala);
        if (!coordinate.empty()) {
          partenza_mondo = coordinate[0];
          partenza_trovata = true;
        }
      }
      if (id == id_arrivo) {
        std::vector<Point> coordinate =
            estrai_coordinate(blocco, fattore_scala);
        if (!coordinate.empty()) {
          arrivo_mondo = coordinate[0];
          arrivo_trovato = true;
        }
      }
      if (id != -1 && id_vietati.contains(id)) {
        std::vector<Point> coordinate =
            estrai_coordinate(blocco, fattore_scala);
        bool pienezza = (id_pieni.contains(id));
        if (!coordinate.empty()) {
          oggetti_vietati.push_back({id, coordinate, pienezza});
        }
      }
    }
  }
  double min_x = 0, max_x = 0, min_y = 0, max_y = 0;
  calcola_dimensioni_max_min(oggetti_vietati, min_x, max_x, min_y, max_y,
                             arrivo_mondo, partenza_mondo);
  int risoluzione = 800; // risoluzione, buona via di mezzo, si potrebbe mettere
                         // in funzione scala-x_min, x_max
  double larghezza_mondo = max_x - min_x;
  double altezza_mondo = max_y - min_y;

  int larghezza_griglia = risoluzione;
  int altezza_griglia =
      static_cast<int>(risoluzione * (altezza_mondo / larghezza_mondo));
  if (altezza_griglia < 1) {
    altezza_griglia = risoluzione;
  }
  assert(larghezza_griglia > 0);
  assert(altezza_griglia > 0);

  std::vector<std::vector<bool>> griglia(
      static_cast<size_t>(altezza_griglia),
      std::vector<bool>(static_cast<size_t>(larghezza_griglia), true));
  for (const auto &ogg : oggetti_vietati) {
    oggetti_to_griglia(ogg, griglia, min_x, max_x, min_y, max_y,
                       larghezza_griglia, altezza_griglia); // prima gli oggetti
    /*std::cout << "  Rasterizzato ID=" << ogg.id << " (" << ogg.contorno.size()
              << " punti, " << (ogg.e_pieno ? "pieno" : "linea") << ")\n";*/
  }
  dilata_ostacoli(griglia);
  int partenza_gx = 0, partenza_gy = 0;
  int arrivo_gx = 0, arrivo_gy = 0;
  if (partenza_trovata) {
    mondo_a_griglia(partenza_mondo.x, partenza_mondo.y, min_x, max_x, min_y,
                    max_y, larghezza_griglia, altezza_griglia, partenza_gx,
                    partenza_gy);
  }

  if (arrivo_trovato) {
    mondo_a_griglia(arrivo_mondo.x, arrivo_mondo.y, min_x, max_x, min_y, max_y,
                    larghezza_griglia, altezza_griglia, arrivo_gx, arrivo_gy);
  }

  assert(partenza_trovata);
  assert(arrivo_trovato);

  Point partenza(partenza_gx, partenza_gy);
  Point arrivo(arrivo_gx, arrivo_gy);
  Grid grid(griglia);

  std::vector<Path> paths = final_aggiratore(partenza, arrivo, griglia);

  /*std::cout << partenza_trovata << "\n";
  std::cout << "coordinate partenza" << partenza_gx << "," << partenza_gy
            << "\n coordinate arrivo" << arrivo_gx << "," << arrivo_gy <<
  "\n";*/
  std::cout << "Trovati " << paths.size()
            << " percorsi entro +20% del migliore:\n\n";
  std::cout << "Percorsi di lunghezza:\n";
  for (size_t k = 0; k < paths.size(); k++) {
    std::cout << costo_path_geometrico_mondo(paths[k].points, min_x, max_x,
                                             min_y, max_y, larghezza_griglia,
                                             altezza_griglia, fattore_scala)
              << "m\n";
  }
  std::ofstream csv("griglia.csv");
  if (csv.is_open()) {
    csv << altezza_griglia << "," << larghezza_griglia << "\n";
    for (int y = 0; y < altezza_griglia; ++y) {
      for (int x = 0; x < larghezza_griglia; ++x) {
        if (partenza_trovata && x == partenza_gx && y == partenza_gy) {
          csv << "S";
        } else if (arrivo_trovato && x == arrivo_gx && y == arrivo_gy) {
          csv << "A";
        } else {
          csv << (griglia[static_cast<size_t>(y)][static_cast<size_t>(x)]
                      ? "0"
                      : "1");
        }
        if (x < larghezza_griglia - 1) {
          csv << ",";
        }
      }
      csv << "\n";
    }

    csv.close();
  } else {
    std::cout << "Problemi nella creazione del file csv \n";
  }
  if (!paths.empty()) {
    esporta_percorso_omap(nome_file, "mappa_con_percorso.omap", paths,
                          ids_symbols, min_x, max_x, min_y, max_y,
                          larghezza_griglia, altezza_griglia, fattore_scala);
  }
  return paths;
}
} // namespace pf