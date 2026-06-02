// Per spiegare l'algoritmo viene prima data un'idea generale e poi qualche
// funzione viene trattata più nel dettaglio. Il pathfinding parte controllando
// se si può andare in linea retta verso l'arrivo e quindi controllando se sono
// liberi tutti i punti da partenza ad arrivo, utilizzando l'algoritmo di
// Bresenham. Se non sono liberi, l'algoritmo si ferma al primo ostacolo che
// incontra e trova il punto prima del primo ostacolo che chiameremo P_0. Si
// divide quindi nel percorso che gira in senso orario e in quello che gira in
// senso antiorario. Per ognuno dei due rami segue il bordo fino a che non vede
// spazio in direzione dell'arrivo, ovvero finchè il primo bool in direzione non
// è libero. Si salva quel punto che chiameremo C. Segue il bordo all'indietro
// finchè non vede spazio in direzione della partenza e si salva quel punto che
// chiameremo P e tutti i punti del bordo tra P e C. Crea il percorso formato da
// partenza, P, C con tutti i punti della griglia della retta partenza-> P e
// tutti i punti del bordo.

// A questo punto viene riapplicata la funzione intera con, come partenza, C e,
// come arrivo, sempre l'arrivo.

// Quando l'ultimo C vedrà l'arrivo e non solo spazio in direzione si chiuderà
// il percorso che verrà pulito: verranno tolti tutti i nodi in eccesso e le
// deviazioni inutili. E se la retta P->partenza o
// P -> C_precedente non è completamente libera [perchè,
// ricordo, il mio algoritmo passa per P_0 (e non lo salva, il percorso dalla
// partenza va diretto a P)] viene riapplicato l'algoritmo con, come partenza,
// la partenza stessa e, come arrivo, il punto prima di quello che non vede
// punti precedenti, per essere certi di avere tutti i percorsi.

// Una volta che si hanno tutti i percorsi si ricalcola la lunghezza del
// percorso per sicurezza e si filtrano quelli lunghi più del 120\% del percorso
// più corto.

// Funzioni particolari
//  Per seguire il bordo c'è un movimento 8-direzionale con priorità. Quindi si
//  cerca da che parte è l'ostacolo e si prova prima ad andare in quella
//  direzione poi, se è in senso orario, per esempio, si girerà verso sinistra
//  fino a che non c'è una cella libera in quella direzione. Si controlla di
//  avere l'ostacolo vicino e, se si vede spazio in direzione dell'arrivo, si
//  prosegue.

// Per pulire i percorsi si parte dall'arrivo e si cerca il punto più lontano
// che vede, quindi partendo dalla partenza e poi avvicindandosi all'arrivo. Se
// se ne vede uno vengono tolti tutti i punti tra i due che si vedono. Se non se
// ne vede nessuno invece si applica nuovamente l'algoritmo come spiegato sopra.

// La funzione che pulisce viene applicata finchè non si ottiene due volte di
// fila lo stesso percorso. Questo serve per pulire anche la zona dell'unione
// tra i paths trovati apllicando l'algoritmo tra la partenza e quei punti che
// non vedono punti prima, e la fine del percorso. Infatti entrambi i tratti
// sono "puliti" ma può essere tolto qualcosa nell'unione.

#include "path.hpp"
#include <algorithm>
#include <cassert>
#include <cmath>
#include <iostream>
#include <queue>
#include <vector>
namespace pf {
const int DX[4] = {0, 1, 0, -1};
const int DY[4] = {-1, 0, 1, 0};

// costruisce una griglia a partire da una matrice booleana (true = cella
// libera, false = ostacolo) inizializza matrice degli id degli ostacoli a -1
Grid::Grid(const std::vector<std::vector<bool>> &g) : griglia(g) {
  assert(!g.empty());
  assert(!g[0].empty());
  altezza = static_cast<int>(g.size());
  larghezza = (altezza > 0) ? static_cast<int>(g[0].size()) : 0;
  id_ostacolo.resize(static_cast<size_t>(altezza),
                     std::vector<int>(static_cast<size_t>(larghezza), -1));
}

bool Grid::is_free(int x, int y) const { // ritorna valore cella, se nei margini
  if (x < 0 || x >= larghezza || y < 0 || y >= altezza) {
    return false;
  }
  return griglia[static_cast<size_t>(y)][static_cast<size_t>(x)];
}
bool Grid::is_free(const Point &p) const { return is_free(p.x, p.y); }
int Grid::get_larghezza() const { return larghezza; }
int Grid::get_altezza() const { return altezza; }

// assegna un id univoco a ogni ostacolo, stesso ostacolo stesso id
void Grid::controlla_id_ostacolo() {
  assert(larghezza > 0 && altezza > 0);
  int prossimo_id = 0;
  for (int y = 0; y < altezza; ++y) {
    for (int x = 0; x < larghezza; ++x) {
      if (!is_free(x, y) &&
          id_ostacolo[static_cast<size_t>(y)][static_cast<size_t>(x)] == -1) {
        riempi(x, y, prossimo_id);
        ++prossimo_id;
      }
    }
  }
}

// restituisce id dell'ostacolo, -1 per il resto
int Grid::get_id_ostacolo(int x, int y) const {
  if (x < 0 || x >= larghezza || y < 0 || y >= altezza) {
    return -1;
  }
  return id_ostacolo[static_cast<size_t>(y)][static_cast<size_t>(x)];
}
int Grid::get_id_ostacolo(const Point &p) const {
  return get_id_ostacolo(p.x, p.y);
}

// floodfill
void Grid::riempi(int start_x, int start_y, int id) {
  assert(start_x >= 0 && start_x < larghezza);
  assert(start_y >= 0 && start_y < altezza);
  assert(id >= 0);
  std::queue<Point> q;
  q.push(Point(start_x, start_y));
  id_ostacolo[static_cast<size_t>(start_y)][static_cast<size_t>(start_x)] = id;
  while (!q.empty()) {
    Point p = q.front();
    q.pop();
    for (int d = 0; d < 4; ++d) {
      int nx = p.x + DX[d], ny = p.y + DY[d];
      if (nx >= 0 && nx < larghezza && ny >= 0 && ny < altezza) {
        if (!is_free(nx, ny) &&
            id_ostacolo[static_cast<size_t>(ny)][static_cast<size_t>(nx)] ==
                -1) {
          id_ostacolo[static_cast<size_t>(ny)][static_cast<size_t>(nx)] = id;
          q.push(Point(nx, ny));
        }
      }
    }
  }
}

// linea dirtta da a a b con algoritmo bresenham
std::vector<Point> linea_dritta(const Point &da, const Point &a) {
  std::vector<Point> punti;
  int x0 = da.x, y0 = da.y, x1 = a.x, y1 = a.y;
  int dx = std::abs(x1 - x0), dy = std::abs(y1 - y0);
  int sx = x0 < x1 ? 1 : -1, sy = y0 < y1 ? 1 : -1;
  int err = dx - dy;
  while (true) {
    punti.push_back(Point(x0, y0));
    if (x0 == x1 && y0 == y1) {
      break;
    }
    int e2 = 2 * err;
    if (e2 > -dy) {
      err -= dy;
      x0 += sx;
    }
    if (e2 < dx) {
      err += dx;
      y0 += sy;
    }
  }
  return punti;
}

// primo punto del primo ostacolo sotto la linea retta
Point primo_ostacolo_sulla_linea(const Point &A, const Point &B,
                                 const Grid &griglia) {
  std::vector<Point> linea = linea_dritta(A, B);
  assert(!linea.empty());
  for (size_t i = 1; i < linea.size(); ++i) {
    Point p = linea[i];
    if (p == B) {
      continue;
    }
    if (!griglia.is_free(p)) {
      return p;
    }
  }
  return Point(-1, -1);
}

// ultimo punto libero prima dell'ostacolo
Point ultimo_punto_prima_del_primo_ostacolo(const Point &A, const Point &B,
                                            const Grid &griglia) {
  std::vector<Point> linea = linea_dritta(A, B);
  assert(!linea.empty());
  Point ultimo_libero = A;
  for (size_t i = 1; i < linea.size(); ++i) {
    if (!griglia.is_free(linea[i])) {
      return ultimo_libero;
    }
    ultimo_libero = linea[i];
  }
  return B;
}

// dal punto(P) c'è linea dritta libera verso arrivo(B)?
bool vede_punto_di_arrivo(const Point &P, const Point &B, const Grid &griglia) {
  std::vector<Point> linea = linea_dritta(P, B);
  assert(!linea.empty());
  for (size_t i = 1; i < linea.size(); ++i) {
    if (!griglia.is_free(linea[i])) {
      return false;
    }
  }
  return true;
}

// dal punto(P) c'è linea dritta libera per 1 bool in direzione
// dell'arrivo(B)(non entra in loop con concavi)
bool vede_spazio_in_direzione_arrivo(const Point &P, const Point &B,
                                     const Grid &griglia, int id_ostacolo) {
  assert(id_ostacolo >= 0);
  std::vector<Point> linea = linea_dritta(P, B);
  assert(!linea.empty());
  for (size_t i = 1; i < linea.size(); ++i) {
    Point p = linea[i];
    if (!griglia.is_free(p)) {
      int id_colpito = griglia.get_id_ostacolo(p);
      if (id_colpito == id_ostacolo) {
        return false;
      } else {
        return true;
      }
    }
    if (p == B) {
      return true;
    }
  }
  return true;
}

//=============================================funzione sefui il bordo
Border_exit segui_il_bordo_monodirezione(const Point &partenza,
                                         const Point &destinazione,
                                         const Grid &griglia, int id_ostacolo,
                                         senso_di_percorrenza direzione) {

  assert(id_ostacolo >= 0);
  assert(griglia.is_free(partenza));

  Border_exit risultato{};

  const int MAX_PASSI = 4000; // limite di passi segui bordo

  // 8 direzioni
  static const int DX8[8] = {0, 1, 1, 1, 0, -1, -1, -1};
  static const int DY8[8] = {-1, -1, 0, 1, 1, 1, 0, -1};

  auto diagonale_valida = [&](Point from,
                              Point to, // lambda per evitare tagli degli angoli
                              const Grid &g) -> bool {
    int dx = to.x - from.x;
    int dy = to.y - from.y;

    // solo diagonali
    if (std::abs(dx) == 1 && std::abs(dy) == 1) {

      Point p1(from.x + dx, from.y);
      Point p2(from.x, from.y + dy);

      // taglio angoli
      if (!g.is_free(p1) && !g.is_free(p2)) {
        return false;
      }
    }

    return true;
  };

  Point corrente = partenza;

  Path percorso;
  percorso.points.push_back(corrente);

  int dir_precedente = -1;

  // trova primo orientamento rispetto all'ostacolo

  for (int d = 0; d < 8; ++d) {

    Point vicino(partenza.x + DX8[d], partenza.y + DY8[d]);

    if (!griglia.is_free(vicino) &&
        griglia.get_id_ostacolo(vicino) == id_ostacolo) {

      if (direzione == orario) {
        dir_precedente = (d + 6) % 8;
      } else {
        dir_precedente = (d + 2) % 8;
      }

      break;
    }
  }

  // non lo trova -> riprova a distanza 2

  if (dir_precedente == -1) {

    bool trovato = false;

    for (int d = 0; d < 8; ++d) {

      Point vicino(partenza.x + DX8[d], partenza.y + DY8[d]);

      if (!griglia.is_free(vicino)) {
        continue;
      }

      if (!diagonale_valida(partenza, vicino, griglia)) {
        continue;
      }

      for (int k = 0; k < 8; ++k) {

        Point adiacente(vicino.x + DX8[k], vicino.y + DY8[k]);

        if (!griglia.is_free(adiacente) &&
            griglia.get_id_ostacolo(adiacente) == id_ostacolo) {

          corrente = vicino;
          percorso.points.push_back(corrente);

          if (direzione == orario) {
            dir_precedente = (k + 6) % 8;
          } else {
            dir_precedente = (k + 2) % 8;
          }

          trovato = true;
          break;
        }
      }

      if (trovato) {
        break;
      }
    }

    if (!trovato) {
      return risultato;
    }
  }

  int passi = 0;

  // segui il bordo vero e proprio
  bool uscita_trovata = false;
  int passi_rimanenti = 0;
  while (passi < MAX_PASSI) {
    if (uscita_trovata) {

      --passi_rimanenti;

      if (passi_rimanenti == 0) {

        risultato.exit = corrente;
        risultato.bordo = percorso;
        risultato.direzione = direzione;

        return risultato;
      }
    }
    // uscita valida
    if (!uscita_trovata && vede_spazio_in_direzione_arrivo(
                               corrente, destinazione, griglia, id_ostacolo)) {
      uscita_trovata = true;
      passi_rimanenti = 8;
    }

    int priorita[8];

    // priorità orario e antioriario, prima verso ostacolo, poi nella direzione
    // orario/antiorario

    if (direzione == orario) {

      priorita[0] = (dir_precedente + 2) % 8;
      priorita[1] = (dir_precedente + 1) % 8;
      priorita[2] = dir_precedente;
      priorita[3] = (dir_precedente + 7) % 8;
      priorita[4] = (dir_precedente + 6) % 8;
      priorita[5] = (dir_precedente + 5) % 8;
      priorita[6] = (dir_precedente + 4) % 8;
      priorita[7] = (dir_precedente + 3) % 8;
    }

    else {

      priorita[0] = (dir_precedente + 6) % 8;
      priorita[1] = (dir_precedente + 7) % 8;
      priorita[2] = dir_precedente;
      priorita[3] = (dir_precedente + 1) % 8;
      priorita[4] = (dir_precedente + 2) % 8;
      priorita[5] = (dir_precedente + 3) % 8;
      priorita[6] = (dir_precedente + 4) % 8;
      priorita[7] = (dir_precedente + 5) % 8;
    }

    bool spostato = false;

    for (int i = 0; i < 8; ++i) {

      int tdir = priorita[i];

      Point prossimo(corrente.x + DX8[tdir], corrente.y + DY8[tdir]);

      // deve essere libero
      if (!griglia.is_free(prossimo)) {
        continue;
      }

      // evita attraversamento angoli
      if (!diagonale_valida(corrente, prossimo, griglia)) {
        continue;
      }

      // deve toccare l'ostacolo
      bool tocca_ostacolo = false;

      for (int k = 0; k < 8; ++k) {

        Point adiacente(prossimo.x + DX8[k], prossimo.y + DY8[k]);

        if (!griglia.is_free(adiacente) &&
            griglia.get_id_ostacolo(adiacente) == id_ostacolo) {

          tocca_ostacolo = true;
          break;
        }
      }

      if (!tocca_ostacolo) {
        continue;
      }

      // lo muove
      corrente = prossimo;

      percorso.points.push_back(corrente);

      dir_precedente = tdir;

      spostato = true;

      break;
    }

    if (!spostato) {
      break;
    }

    ++passi;
  }

  return Border_exit();
}

senso_di_percorrenza direzione_opposta(senso_di_percorrenza d) {
  return (d == orario) ? antiorario : orario;
}

double costo_path_geometrico(
    const std::vector<Point>
        &path) { // per ogni punto fa pitagora tra punto e quello prima, non è
                 // costo effettivo ma mantiene proprzioni
  assert(path.size() >= 1);
  double cost = 0.0;
  for (size_t i = 1; i < path.size(); ++i) {
    cost += std::hypot(path[i].x - path[i - 1].x, path[i].y - path[i - 1].y);
  }
  return cost;
}

// pulisce e genera nuovi percorsi se necessario
std::vector<std::vector<Point>>
pulisci_all_indietro(const std::vector<Point> &path, const Grid &griglia) {

  assert(!path.empty());

  std::vector<Point> corrente = path;

  if (corrente.size() <= 2) {
    return {corrente}; // se 2 punti non potrà mai tagliare
  }

  bool cambiato = true; // per vedere se continuare a pulire

  while (cambiato) {

    cambiato = false;

    int i = static_cast<int>(corrente.size()) - 1;

    while (i > 1) {

      int best = -1;

      // cerca il punto più vecchio visibile
      for (int j = 0; j < i; ++j) {
        if (vede_punto_di_arrivo(corrente[static_cast<size_t>(i)],
                                 corrente[static_cast<size_t>(j)], griglia)) {
          best = j; // se lo trova esce e se lo salva
          break;
        }
      }

      if (best == i - 1) {
        --i;
        continue; // se arriva al punto prima e lo vede, tutto ok ma non taglia
                  // niente
      }

      if (best !=
          -1) { // se ne vede uno prima taglia, elimina tutti i punti in mezzo

        std::vector<Point> nuovo;

        nuovo.insert(nuovo.end(), corrente.begin(),
                     corrente.begin() + best + 1);

        nuovo.push_back(corrente[static_cast<size_t>(i)]);

        if (!(i + 1 == static_cast<int>(corrente.size()))) {
          nuovo.insert(nuovo.end(), corrente.begin() + i + 1, corrente.end());
        }

        if (nuovo != corrente) {
          corrente = nuovo;
          cambiato = true;

          // riparti da capo sul nuovo percorso
          break;
        }

        i = best;
      } else { // se non vede nessun punto prima-> capita tra partenza/corrente
               // e P

        std::vector<Path> paths_back = trova_paths(
            corrente[0], corrente[static_cast<size_t>(i + 1)],
            griglia); // riapplica tutto l'algoritmo tra la partenza e il punto
                      // prima di quello che non vede altri punti dopo

        if (paths_back.empty()) {
          return {};
        }

        std::vector<std::vector<Point>> risultati;

        for (const auto &pb : paths_back) {

          std::vector<Point> finale = pb.points;

          finale.insert(finale.end(), corrente.begin() + i + 1,
                        corrente.end()); // lo unisce ai punti che c'erano prima
          risultati.push_back(finale);
        }

        return risultati;
      }
    }
  }

  return {corrente};
}

std::vector<std::vector<Point>> // funzione per pulire finchè un percorso cambia
pulisci_fino_a_stabile(const std::vector<Point> &path, const Grid &griglia) {
  assert(!path.empty());

  std::vector<std::vector<Point>> correnti = {path};

  bool cambiato = true;

  while (cambiato) {
    cambiato = false;

    std::vector<std::vector<Point>> nuovi;

    for (const auto &p : correnti) {
      auto puliti = pulisci_all_indietro(p, griglia);

      if (puliti.size() != 1 || puliti[0] != p) {
        cambiato = true;
      }

      nuovi.insert(nuovi.end(), puliti.begin(), puliti.end());
    }

    correnti.swap(nuovi); // per non copiare tutto
  }

  return correnti;
}

//=========================================funzione ricorsiva che crea i
// percorsi
void costruisci_percorsi(const Point &corrente, const Point &destinazione,
                         const Grid &griglia,
                         std::vector<Point> &percorso_parziale,
                         double costo_parziale,
                         std::vector<Path> &percorsi_output, int profondita) {
  const int MAX_PROFONDITA = 8; // + di 8 volte rischia di esplodere
  assert(profondita >= 0);
  assert(corrente.e_valido() && destinazione.e_valido());

  if (profondita > MAX_PROFONDITA) {
    return;
  }

  if (vede_punto_di_arrivo(corrente, destinazione,
                           griglia)) { // se vede l'arrivo pulisce e return
    std::vector<Point> retta_finale{corrente, destinazione};
    Path p;
    p.points = percorso_parziale;
    p.points.insert(p.points.end(), retta_finale.begin(), retta_finale.end());
    p.cost = costo_parziale + costo_path_geometrico(retta_finale);
    // percorsi_output.push_back(p);//da attivare se si toglie
    // pulisci_fino_a_stabile
    //   pulisci ritorna una o più varianti
    auto varianti = pulisci_fino_a_stabile(p.points, griglia);
    if (!varianti.empty()) {
      for (const auto &variante : varianti) {
        Path pv;
        pv.points = variante;
        // pv.points.insert(pv.points.end(), retta_finale.begin(),
        //                  retta_finale.end());
        // pv.cost = costo_parziale + costo_path_geometrico(retta_finale);
        percorsi_output.push_back(pv);
      }
    }
    return;
  }

  Point primo_colpito =
      primo_ostacolo_sulla_linea(corrente, destinazione, griglia);
  if (!primo_colpito.e_valido()) {
    return;
  }
  assert(primo_colpito.e_valido());

  int id_ost = griglia.get_id_ostacolo(primo_colpito);
  assert(id_ost >= 0);

  Point P0 =
      ultimo_punto_prima_del_primo_ostacolo(corrente, destinazione, griglia);
  bool adiacente = false;
  for (int d = 0; d < 8; ++d) { // controlla sa vicino a ostacolo
    static const int DX8[8] = {0, 1, 1, 1, 0, -1, -1, -1};
    static const int DY8[8] = {-1, -1, 0, 1, 1, 1, 0, -1};
    Point nb(P0.x + DX8[d], P0.y + DY8[d]);
    if (!griglia.is_free(nb) && griglia.get_id_ostacolo(nb) == id_ost) {
      adiacente = true;
      break;
    }
  }
  if (!adiacente) { // altrimenti prova da primo_colpito le 4 direzioni e ne
                    // trova una dove è libero
    for (int d = 0; d < 4; ++d) {
      Point nb(primo_colpito.x + DX[d], primo_colpito.y + DY[d]);
      if (griglia.is_free(nb)) {
        P0 = nb;
        break;
      }
    }
  }

  for (int d = 0; d < 2; ++d) { // per ogni senso di percorrenza
    senso_di_percorrenza dir = ((d == 0) ? orario : antiorario);
    senso_di_percorrenza dir_inversa = direzione_opposta(dir);
    Border_exit uscita_dest =
        segui_il_bordo_monodirezione(P0, destinazione, griglia, id_ost,
                                     dir); // segui il bordo(fino a uscita(C))
    if (!uscita_dest.exit.e_valido()) {
      continue;
    }
    Point C = uscita_dest.exit;

    Border_exit uscita_corr = // torna indietro e trova P
        segui_il_bordo_monodirezione(C, corrente, griglia, id_ost, dir_inversa);
    if (!uscita_corr.exit.e_valido()) {
      continue;
    }
    Point P = uscita_corr.exit;
    // P -> C segue davvero il bordo

    std::vector<Point> bordo_P_C = uscita_corr.bordo.points;
    std::reverse(bordo_P_C.begin(), bordo_P_C.end());
    /*for (int i = 0; i < static_cast<int>(bordo_P_C.size()); ++i) {
      std::cout << "(" << bordo_P_C[static_cast<size_t>(i)].x << "; "
                << bordo_P_C[static_cast<size_t>(i)].y << "), ";
    }*/

    // costruzione del path
    std::vector<Point> nuovo_percorso =
        percorso_parziale; // è ricorsiva quindi parte già da un pezzo fatto

    // 1. corrente -> P (linea con tutti i punti della griglia, non solo
    // estremi)
    std::vector<Point> seg_corr_P = linea_dritta(corrente, P);
    if (!seg_corr_P.empty()) {
      if (!nuovo_percorso.empty() &&
          nuovo_percorso.back() == seg_corr_P.front()) {
        seg_corr_P.erase(seg_corr_P.begin());
      }
      nuovo_percorso.insert(nuovo_percorso.end(), seg_corr_P.begin(),
                            seg_corr_P.end());
    }

    // 2. bordo P -> C (sempre tutti i punti)
    if (!bordo_P_C.empty()) {
      // evita duplicato tra fine segmento e inizio bordo
      if (!nuovo_percorso.empty() &&
          nuovo_percorso.back() == bordo_P_C.front()) {
        bordo_P_C.erase(bordo_P_C.begin());
      }

      nuovo_percorso.insert(nuovo_percorso.end(), bordo_P_C.begin(),
                            bordo_P_C.end());
    }
    // calcola il costo, inutile tanto dopo verrà rifatto ma potrebbe diventarlo
    // se si filtra nel mentre
    double nuovo_costo = costo_parziale;

    // corrente -> P
    for (size_t i = 1; i < seg_corr_P.size(); ++i) {
      nuovo_costo += std::hypot(seg_corr_P[i].x - seg_corr_P[i - 1].x,
                                seg_corr_P[i].y - seg_corr_P[i - 1].y);
    }

    // bordo P -> C
    for (size_t i = 1; i < bordo_P_C.size(); ++i) {
      nuovo_costo += std::hypot(bordo_P_C[i].x - bordo_P_C[i - 1].x,
                                bordo_P_C[i].y - bordo_P_C[i - 1].y);
    }

    costruisci_percorsi(C, destinazione, griglia, nuovo_percorso, nuovo_costo,
                        percorsi_output, profondita + 1);
  }
}

// se è dritto va, se no costruisvi_percorsi, ordina per costo da min a max,
// elimina quelli uguali
std::vector<Path> trova_paths(const Point &A, const Point &B,
                              const Grid &griglia) {
  assert(A.e_valido() && B.e_valido());
  assert(griglia.is_free(A) && griglia.is_free(B));
  std::vector<Path> risultati;
  if (vede_punto_di_arrivo(A, B, griglia)) {
    Path diretto;
    diretto.points = {A, B};
    diretto.cost = costo_path_geometrico(linea_dritta(A, B));
    return {diretto};
  }
  std::vector<Point> base{};
  costruisci_percorsi(A, B, griglia, base, 0, risultati, 0);

  std::sort(risultati.begin(), risultati.end(),
            [](const Path &a, const Path &b) { return a.points < b.points; });
  risultati.erase(std::unique(risultati.begin(), risultati.end(),
                              [](const Path &a, const Path &b) {
                                return a.points == b.points;
                              }),
                  risultati.end());

  return risultati;
}

std::vector<Path> filtra_paths(
    std::vector<Path> &percorsi) { // ricalcola costi e trova min, poi filtra
  if (percorsi.empty()) {
    return {};
  }
  assert(!percorsi.empty());

  // ricalcola costi per sicurezza
  for (auto &p : percorsi) {
    p.cost = costo_path_geometrico(
        p.points); // non è costo effettivo ma mantiene proprzioni quindi si può
                   // usare per filtrare
  }

  auto it = std::min_element(
      percorsi.begin(), percorsi.end(),
      [](const Path &a, const Path &b) { return a.cost < b.cost; });
  double costo_minimo = it->cost;
  double costo_massimo = costo_minimo * 1.2;

  std::vector<Path> filtrati;
  std::copy_if(percorsi.begin(), percorsi.end(), std::back_inserter(filtrati),
               [&](const Path &p) { return p.cost <= costo_massimo; });
  return filtrati;
}
} // namespace pf