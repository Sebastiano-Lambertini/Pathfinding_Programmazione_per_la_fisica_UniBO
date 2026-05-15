#include "path.prova4.hpp"
#include <fstream>
#include <iostream>
#include <string>
#include <vector>

std::vector<Path>
final_aggiratore(Point A, Point B,
                 const std::vector<std::vector<bool>> &griglia) {
  Grid grid(griglia);
  grid.controlla_id_ostacolo();
  std::set<int> visited;
  std::vector<Path> allPaths = trova_paths(A, B, grid, visited, 0);
  std::vector<Path> paths = filtra_paths(allPaths);
  return paths;
}

struct id_to_symbol {
  int id;
  int symbol;

  id_to_symbol() : id(0), symbol(0) {}

  id_to_symbol(int id_val, int symbol_val) : id(id_val), symbol(symbol_val) {}
};

struct oggetto_poligonale_vietato {
  int id;
  std::vector<Point> contorno;
  bool e_pieno;
  bool e_curva;
};

size_t salta_virgolette_spazi(const std::string &linea, size_t inizio) {
  while (inizio < linea.size() &&
         (linea[inizio] == '"' || linea[inizio] == ' ')) {
    ++inizio;
  }
  return inizio;
}

size_t salta_spazi(const std::string &linea, size_t inizio) {
  while (inizio < linea.size() && (linea[inizio] == ' ')) {
    ++inizio;
  }
  return inizio;
}

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

id_to_symbol ids_to_symbols(std::string linea) {
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

std::vector<int>
trova_id_vietati_pieni(const std::vector<id_to_symbol> &ids_symbols,
                       const std::vector<int> &vietati) {
  std::vector<int> risultato{};
  for (const auto &coppia : ids_symbols) {
    for (int simbolo_vietato : vietati) {
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
  return scala / 1000000;
}

std::vector<Point> estrai_coordinate(const std::string &linea,
                                     const double fattore_scala) {
  std::vector<Point> punti{};
  std::string stringa_coordinate{};
  auto inizio = linea.find("<coords");
  if (inizio != std::string::npos) {
    inizio = linea.find(">", inizio) + 1;
    auto fine = linea.find("</coords>", inizio);
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

  // Applica fattore di scala
  for (auto &pf : punti_flag) {
    pf.x *= fattore_scala;
    pf.y *= fattore_scala;
  }

  // ultimo punto ha token 18--->chiusa
  bool chiusa = false;
  if (!punti_flag.empty()) {
    if (punti_flag.back().flag == 18) {
      chiusa = true;
    } else {
      double dx = punti_flag.back().x - punti_flag.front().x;
      double dy = punti_flag.back().y - punti_flag.front().y;
      if (dx * dx + dy * dy < 1e-6) {
        chiusa = true;
      }
    }
  }

  // quanti nodi di curva ci sono
  std::vector<int> indici_nodi;
  for (size_t i = 0; i < punti_flag.size(); ++i) {
    if (punti_flag[i].flag == 1) {
      indici_nodi.push_back(static_cast<int>(i));
    }
  }

  // Se 0, normale
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
    //        std::cout<<"curva bezier cubica\n";
    return {x, y, 0};
  };

  // Divide in n segmentini la curva tra due nodi
  auto aggiungi_segmento = [&](int inizio_idx, int fine_idx) {
    const auto &nodo_inizio = punti_flag[inizio_idx];
    const auto &nodo_fine = punti_flag[fine_idx];
    int num_nodi_di_curva = fine_idx - inizio_idx - 1;

    if (num_nodi_di_curva == 2) {
      // Curva cubica
      const auto &nodo_di_curva_1 = punti_flag[inizio_idx + 1];
      const auto &nodo_di_curva_2 = punti_flag[inizio_idx + 2];
      const int passi = 10;
      for (int s = 1; s <= passi; ++s) {
        double t = static_cast<double>(s) / passi;
        auto pt = bezier_cubica(nodo_inizio, nodo_di_curva_1, nodo_di_curva_2,
                                nodo_fine, t);
        punti.push_back({static_cast<int>(pt.x), static_cast<int>(pt.y)});
      }
    } else {
      // Se problemi fa linea retta
      for (int i = inizio_idx + 1; i <= fine_idx; ++i) {
        punti.push_back({static_cast<int>(punti_flag[i].x),
                         static_cast<int>(punti_flag[i].y)});
      }
    }
  };

  // Aggiunge primo nodo
  punti.push_back({static_cast<int>(punti_flag[indici_nodi[0]].x),
                   static_cast<int>(punti_flag[indici_nodi[0]].y)});

  // e segmenti
  for (size_t k = 0; k < indici_nodi.size() - 1; ++k) {
    aggiungi_segmento(indici_nodi[k], indici_nodi[k + 1]);
  }

  // Se chiusa, collega l'ultimo nodo al primo (per sicurezza, per fare anche
  // curva finale)
  /*if (chiusa) {
    int ultimo_nodo = indici_nodi.back();
    int primo_nodo  = indici_nodi.front();
    // Costruisce la sequenza di punti che parte dall'ultimo nodo e arriva al
  primo,
    // includendo eventuali punti dopo l'ultimo nodo e prima del primo nodo.
    std::vector<PuntoFlag> punti_avvolti;
    punti_avvolti.push_back(punti_flag[ultimo_nodo]);
    for (int i = ultimo_nodo + 1; i < static_cast<int>(punti_flag.size()); ++i)
      punti_avvolti.push_back(punti_flag[i]);
    for (int i = 0; i <= primo_nodo; ++i)
      punti_avvolti.push_back(punti_flag[i]);

    // punti_avvolti[0] e  punti_avvolti[3] cubica
    // punti_avvolti[0] e punti_avvolti[2] quadratica
    int num_controlli_avvolti = static_cast<int>(punti_avvolti.size()) - 2;
    if (num_controlli_avvolti == 1) {
      const auto& nodo_inizio = punti_avvolti[0];
      const auto& ctrl = punti_avvolti[1];
      const auto& nodo_fine = punti_avvolti[2];
      const int passi = 10;
      for (int s = 1; s <= passi; ++s) {
        double t = static_cast<double>(s) / passi;
        auto pt = bezier_quadratica(nodo_inizio, ctrl, nodo_fine, t);
        punti.push_back({static_cast<int>(pt.x), static_cast<int>(pt.y)});
      }
    } else if (num_controlli_avvolti == 2) {
      const auto& nodo_inizio = punti_avvolti[0];
      const auto& ctrl1 = punti_avvolti[1];
      const auto& ctrl2 = punti_avvolti[2];
      const auto& nodo_fine = punti_avvolti[3];
      const int passi = 10;
      for (int s = 1; s <= passi; ++s) {
        double t = static_cast<double>(s) / passi;
        auto pt = bezier_cubica(nodo_inizio, ctrl1, ctrl2, nodo_fine, t);
        punti.push_back({static_cast<int>(pt.x), static_cast<int>(pt.y)});
      }
    } else {
      // per sicurezza
      for (size_t i = 1; i < punti_avvolti.size(); ++i) {
        punti.push_back({static_cast<int>(punti_avvolti[i].x),
                         static_cast<int>(punti_avvolti[i].y)});
      }
    }
  }*/

  return punti;
}

void calcola_dimensioni_max_min(
    const std::vector<oggetto_poligonale_vietato> &oggetti, double &min_x,
    double &max_x, double &min_y, double &max_y, Point arrivo, Point partenza) {
  min_x = oggetti[0].contorno[0].x;
  max_x = oggetti[0].contorno[0].x;
  min_y = oggetti[0].contorno[0].y;
  max_y = oggetti[0].contorno[0].y;

  for (const auto &oggetto : oggetti) {
    for (const auto &punto : oggetto.contorno) {
      min_x = std::min(min_x, static_cast<double>(punto.x));
      max_x = std::max(max_x, static_cast<double>(punto.x));
      min_y = std::min(min_y, static_cast<double>(punto.y));
      max_y = std::max(max_y, static_cast<double>(punto.y));
    }
  }
  min_x = std::min(min_x, static_cast<double>(arrivo.x));
  max_x = std::max(max_x, static_cast<double>(arrivo.x));
  min_y = std::min(min_y, static_cast<double>(arrivo.y));
  max_y = std::max(max_y, static_cast<double>(arrivo.y));
  min_x = std::min(min_x, static_cast<double>(partenza.x));
  max_x = std::max(max_x, static_cast<double>(partenza.x));
  min_y = std::min(min_y, static_cast<double>(partenza.y));
  max_y = std::max(max_y, static_cast<double>(partenza.y));
  double larghezza = max_x - min_x;
  double altezza = max_y - min_y;
  min_x -= larghezza * 0.05;
  max_x += larghezza * 0.05;
  min_y -= altezza * 0.05;
  max_y += altezza * 0.05;
}

void mondo_a_griglia(double x, double y, double min_x, double max_x,
                     double min_y, double max_y, int larghezza_griglia,
                     int altezza_griglia, int &gx, int &gy) {
  double larghezza_mondo = max_x - min_x;
  double altezza_mondo = max_y - min_y;

  gx = static_cast<int>((x - min_x) / larghezza_mondo * larghezza_griglia);
  gy = static_cast<int>((y - min_y) / altezza_mondo * altezza_griglia);

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
}

bool calcola_intersezione(int x1, int y1, int x2, int y2, int y, int &x) {
  if (y2 != y1) {
    if ((y2 < y && y1 >= y) || (y2 >= y && y1 < y)) {
      x = x1 + (y - y1) * (x2 - x1) / (y2 - y1);
      return true;
    }
  }
  return false;
}

void riempi_poligoni(std::vector<std::vector<bool>> &griglia,
                     const std::vector<std::pair<int, int>> &poligono,
                     int larghezza_griglia, int altezza_griglia) {
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
            griglia[y][x] = false;
          }
        }
      }
    }
  }
}

void verde3_oltrepassabile(std::vector<int> &ids_vietati,
                           std::vector<int> &ids_pieni,
                           std::vector<id_to_symbol> ids_symbols) {
  std::cout << "Il verde 3 è oltrepassabile? (y/n)\n";
  std::string risposta;
  std::cin >> risposta;
  if (risposta != "Y" || risposta != "y" || risposta != "yes" ||
      risposta != "Yes" || risposta != "S" || risposta != "s" ||
      risposta != "sì" || risposta != "si" || risposta != "Sì" ||
      risposta != "Si") {
    ids_vietati.push_back(410);
    ids_pieni.push_back(410);
  }
}

bool is_there_multilevel(std::vector<id_to_symbol> const &ids_symbols) {
  int id_triangolini{-1};
  int id_zona_sotto{-1};

  for (const auto &coppia : ids_symbols) {
    if (coppia.symbol == 512) {
      id_triangolini = coppia.id;
    } else if (coppia.symbol == 511) {
      id_zona_sotto = coppia.id;
    }
  }
  if (id_triangolini == -1 || id_zona_sotto == -1) {
    return false;
  }
  return true;
}

bool is_multilevel_su_percorso(std::vector<Path> const &paths) {
  for (auto path : paths) {
  }
  bool is_under = false;
  return false;
}

void oggetti_to_griglia(const oggetto_poligonale_vietato &oggetto,
                        std::vector<std::vector<bool>> &griglia, double min_x,
                        double max_x, double min_y, double max_y,
                        int larghezza_griglia, int altezza_griglia) {

  if (oggetto.contorno.size() < 2) {
    return;
  }

  auto disegna_linea = [&](int x1, int y1, int x2, int y2) {
    int dx = abs(x2 - x1);
    int dy = abs(y2 - y1);
    int sx = (x1 < x2) ? 1 : -1;
    int sy = (y1 < y2) ? 1 : -1;
    int err = dx - dy;

    int x = x1;
    int y = y1;

    while (true) {
      if (x >= 0 && x < larghezza_griglia && y >= 0 && y < altezza_griglia) {
        griglia[y][x] = false;
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
    mondo_a_griglia(static_cast<double>(punto.x), static_cast<double>(punto.y),
                    min_x, max_x, min_y, max_y, larghezza_griglia,
                    altezza_griglia, gx, gy);
    punti_griglia.push_back({gx, gy});
  }

  for (size_t i = 0; i < punti_griglia.size() - 1; ++i) {
    disegna_linea(punti_griglia[i].first, punti_griglia[i].second,
                  punti_griglia[i + 1].first, punti_griglia[i + 1].second);
  }

  if (oggetto.e_pieno && punti_griglia.size() > 2) {
    riempi_poligoni(griglia, punti_griglia, larghezza_griglia, altezza_griglia);
  }
}

void esporta_percorso_omap(const std::string &filename_input,
                           const std::string &filename_output,
                           const std::vector<Path> &paths,
                           const std::vector<id_to_symbol> &ids_symbols,
                           double min_x, double max_x, double min_y,
                           double max_y, int larghezza_griglia,
                           int altezza_griglia, double fattore_scala) {

  std::ifstream in(filename_input);
  std::ofstream out(filename_output);

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

  bool oggetto_aggiunto = false;

  while (std::getline(in, linea)) {

    if (!oggetto_aggiunto && linea.find("</objects>") != std::string::npos) {
      for (auto path : paths) {
        auto percorso = path.points;
        out << "      <object type=\"1\" symbol=\"" << id_linea_disegno
            << "\">";
        out << "        <coords count=\"" << percorso.size() << "\">";

        for (size_t i = 0; i < percorso.size(); ++i) {
          double x_mondo = min_x + (percorso[i].x + 0.5) * (max_x - min_x) /
                                       larghezza_griglia;
          double y_mondo =
              min_y + (percorso[i].y + 0.5) * (max_y - min_y) / altezza_griglia;

          int x_f = static_cast<int>(x_mondo / fattore_scala);
          int y_f = static_cast<int>(y_mondo / fattore_scala);

          out << (x_f) << " " << (y_f);
          if (i < (percorso.size() - 1)) {
            out << ";";
          } else {
            out << " 16;";
          }
        }

        out << "</coords>";
        out << "<pattern rotation=\"0\"><coord x=\"0\" "
               "y=\"0\"/></pattern>";
        out << "</object>\n";
        oggetto_aggiunto = true;
      }
    }

    out << linea << "\n";
  }

  in.close();
  out.close();
  std::cout << "File OMAP con percorso salvato: " << filename_output << "\n";
}

int main() {
  std::cout << "Inserire nome della mappa:\n";
  std::string nome_file;
  std::cin >> nome_file;
  std::vector<int> vietati{206, 301, 307, 411, 515, 518,
                           520, 521, 708, 709, 714};
  std::vector<int> pieni{206, 301, 307, 411, 520, 521, 709, 714};
  int id_arrivo{0};
  int id_partenza{0};
  bool in_undo = false;
  std::vector<int> ids_vietati{};
  std::vector<int> ids_pieni{};
  std::vector<int> ids{};
  std::vector<id_to_symbol> ids_symbols{};
  Point partenza_mondo;
  Point arrivo_mondo;
  bool partenza_trovata = false;
  bool arrivo_trovato = false;
  std::string linea;
  std::ifstream file(nome_file);
  if (!file.is_open()) {
    std::cout << "File non trovato\n";
    return 1;
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
  verde3_oltrepassabile(vietati, pieni, ids_symbols);
  ids_vietati = trova_id_vietati_pieni(ids_symbols, vietati);
  ids_pieni = trova_id_vietati_pieni(ids_symbols, pieni);
  for (const auto &coppia : ids_symbols) {
    if (coppia.symbol == 706) {
      id_arrivo = coppia.id;
    } else if (coppia.symbol == 701) {
      id_partenza = coppia.id;
    }
  }

  file.clear();
  file.seekg(0);
  while (std::getline(file, linea)) {
    if (linea.find("<undo>") != std::string::npos) {
      in_undo = true;
      continue;
    }
    if (linea.find("</undo>") != std::string::npos) {
      in_undo = false;
      continue;
    }
    if (in_undo) {
      continue;
    }
    if (linea.find("<object") != std::string::npos) {
      int id = trova_simbolo(linea);
      if (id == id_partenza) {
        std::vector<Point> coordinate = estrai_coordinate(linea, fattore_scala);
        if (!coordinate.empty()) {
          partenza_mondo = coordinate[0];
          partenza_trovata = true;
        }
      }
      if (id == id_arrivo) {
        std::vector<Point> coordinate = estrai_coordinate(linea, fattore_scala);
        if (!coordinate.empty()) {
          arrivo_mondo = coordinate[0];
          arrivo_trovato = true;
        }
      }
      if (id != -1 && std::find(ids_vietati.begin(), ids_vietati.end(), id) !=
                          ids_vietati.end()) {
        std::vector<Point> coordinate = estrai_coordinate(linea, fattore_scala);
        bool pienezza = (std::find(ids_pieni.begin(), ids_pieni.end(), id) !=
                         ids_pieni.end());
        if (!coordinate.empty()) {
          oggetti_vietati.push_back({id, coordinate, pienezza, false});
        }
      }
    }
  }
  double min_x = 0, max_x = 0, min_y = 0, max_y = 0;
  calcola_dimensioni_max_min(oggetti_vietati, min_x, max_x, min_y, max_y,
                             arrivo_mondo, partenza_mondo);
  int risoluzione = 800;
  double larghezza_mondo = max_x - min_x;
  double altezza_mondo = max_y - min_y;

  int larghezza_griglia = risoluzione;
  int altezza_griglia = (int)(risoluzione * (altezza_mondo / larghezza_mondo));
  if (altezza_griglia < 1) {
    altezza_griglia = risoluzione;
  }
  std::vector<std::vector<bool>> griglia(
      altezza_griglia, std::vector<bool>(larghezza_griglia, true));
  for (const auto &ogg : oggetti_vietati) {
    oggetti_to_griglia(ogg, griglia, min_x, max_x, min_y, max_y,
                       larghezza_griglia, altezza_griglia);
    std::cout << "  Rasterizzato ID=" << ogg.id << " (" << ogg.contorno.size()
              << " punti, " << (ogg.e_pieno ? "pieno" : "linea") << ")\n";
  }

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
  bool multilevel = is_there_multilevel(ids_symbols);

  Point partenza(partenza_gx, partenza_gy);
  Point arrivo(arrivo_gx, arrivo_gy);
  Grid grid(griglia);
  std::vector<Path> paths = final_aggiratore(partenza, arrivo, griglia);
  std::cout << partenza_trovata << "\n";
  std::cout << "coordinate partenza" << partenza_gx << "," << partenza_gy
            << "\n coordinate arrivo" << arrivo_gx << "," << arrivo_gy << "\n";
  std::cout << "Trovati " << paths.size()
            << " percorsi entro +20% del migliore:\n\n";

  bool multilevel_su_percorso = false;
  if (multilevel) {
    multilevel_su_percorso = is_multilevel_su_percorso(paths);
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
          csv << (griglia[y][x] ? "0" : "1");
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

  std::cout << "ID->SYMBOL\n";
  for (const auto &coppia : ids_symbols) {
    std::cout << "id=" << coppia.id << " -> symbol=" << coppia.symbol << "\n";
  }

  std::cout << "ID VIETATI\n";
  for (int id_vietato : ids_vietati) {
    std::cout << id_vietato << " ";
  }
  std::cout << "\n";
  std::cout << "SIMBOLI VIETATI\n";
  for (int simboli_vietato : vietati) {
    std::cout << simboli_vietato << " ";
  }
  std::cout << "\n";

  std::cout << "OGGETTI VIETATI CON COORDINATE\n";
  for (const auto &ogg : oggetti_vietati) {
    std::cout << "ID=" << ogg.id << ", punti=" << ogg.contorno.size() << "\n";
    for (size_t i = 0; i < ogg.contorno.size(); ++i) {
      std::cout << "  (" << ogg.contorno[i].x << ", " << ogg.contorno[i].y
                << ")\n";
    }
  }

  std::cout << "DIMENSIONI MAPPA-MONDO\n";
  std::cout << "min_x = " << min_x << "\n";
  std::cout << "max_x = " << max_x << "\n";
  std::cout << "min_y = " << min_y << "\n";
  std::cout << "max_y = " << max_y << "\n";
  std::cout << "larghezza_mondo = " << (max_x - min_x) << " m\n";
  std::cout << "altezza_mondo = " << (max_y - min_y) << " m\n";

  std::cout << "DIMENSIONI GRIGLIA\n";
  std::cout << "larghezza_griglia = " << larghezza_griglia << "\n";
  std::cout << "altezza_griglia = " << altezza_griglia << "\n";
  std::cout << "celle totali = " << (larghezza_griglia * altezza_griglia)
            << "\n";

  int vietate = 0;
  for (int y = 0; y < altezza_griglia; ++y) {
    for (int x = 0; x < larghezza_griglia; ++x) {
      if (!griglia[y][x]) {
        ++vietate;
      }
    }
  }
  std::cout << "Numero oggetti vietati: " << oggetti_vietati.size() << "\n";
  int punti_totali = 0;
  for (const auto &ogg : oggetti_vietati) {
    punti_totali += ogg.contorno.size();
  }
  std::cout << "Punti totali: " << punti_totali << "\n";
  std::cout << "Celle vietate: " << vietate << "\n";

  if (!paths.empty()) {
    esporta_percorso_omap(nome_file, "mappa_con_percorso.omap", paths,
                          ids_symbols, min_x, max_x, min_y, max_y,
                          larghezza_griglia, altezza_griglia, fattore_scala);
  }

  return 0;
}

// per smoothare linee dopo si può fare che prendo punti di passaggio, converto
// quelli e li metto dentro