#include "path.prova4.hpp"
#include <queue>
#include <map>
#include <set>
#include <algorithm>
#include <cmath>

const int DX[4] = {0, 1, 0, -1};
const int DY[4] = {-1, 0, 1, 0};
//Funzioni della classe
Grid::Grid(const std::vector<std::vector<bool>> &griglia) : griglia(griglia) {
    altezza = static_cast<int>(griglia.size());
    larghezza = (altezza > 0) ? static_cast<int>(griglia[0].size()) : 0;
    id_ostacolo.resize(altezza, std::vector<int>(larghezza, -1));
}

bool Grid::is_free(int x, int y) const {
    if (x < 0 || x >= larghezza || y < 0 || y >= altezza) return false;
    return griglia[y][x];
}
bool Grid::is_free(const Point &p) const { return is_free(p.x, p.y); }
int Grid::get_larghezza() const { return larghezza; }
int Grid::get_altezza() const { return altezza; }

void Grid::controlla_id_ostacolo() {
    int prossimo_id = 0;
    for (int y = 0; y < altezza; ++y)
        for (int x = 0; x < larghezza; ++x)
            if (!is_free(x, y) && id_ostacolo[y][x] == -1) {
                riempi(x, y, prossimo_id);
                ++prossimo_id;
            }
}

int Grid::get_id_ostacolo(int x, int y) const {
    if (x < 0 || x >= larghezza || y < 0 || y >= altezza) return -1;
    return id_ostacolo[y][x];
}
int Grid::get_id_ostacolo(const Point &p) const { return get_id_ostacolo(p.x, p.y); }

void Grid::riempi(int start_x, int start_y, int id) {
    std::queue<Point> q;
    q.push(Point(start_x, start_y));
    id_ostacolo[start_y][start_x] = id;
    while (!q.empty()) {
        Point p = q.front(); q.pop();
        for (int d = 0; d < 4; ++d) {
            int nx = p.x + DX[d], ny = p.y + DY[d];
            if (nx >= 0 && nx < larghezza && ny >= 0 && ny < altezza)
                if (!is_free(nx, ny) && id_ostacolo[ny][nx] == -1) {
                    id_ostacolo[ny][nx] = id;
                    q.push(Point(nx, ny));
                }
        }
    }
}

//linea dirtta da a a b con algoritmo trovato che non mi ricordo
std::vector<Point> linea_dritta(Point da, Point a) {
    std::vector<Point> punti;
    int x0 = da.x, y0 = da.y, x1 = a.x, y1 = a.y;
    int dx = std::abs(x1 - x0), dy = std::abs(y1 - y0);
    int sx = x0 < x1 ? 1 : -1, sy = y0 < y1 ? 1 : -1;
    int err = dx - dy;
    while (true) {
        punti.push_back(Point(x0, y0));
        if (x0 == x1 && y0 == y1) break;
        int e2 = 2 * err;
        if (e2 > -dy) { err -= dy; x0 += sx; }
        if (e2 < dx)  { err += dx; y0 += sy; }
    }
    return punti;
}

//primo punto del primo ostacolo sotto la linea retta
Point primo_ostacolo_sulla_linea(Point A, Point B, const Grid &griglia) {
    std::vector<Point> linea = linea_dritta(A, B);
    for (size_t i = 1; i < linea.size(); ++i) {
        Point p = linea[i];
        if (p == B) continue;
        if (!griglia.is_free(p)) return p;
    }
    return Point(-1, -1);
}

//ultimo punto libero prima dell'ostacolo
Point ultimo_punto_prima_del_primo_ostacolo(Point A, Point B, const Grid& griglia) {
    std::vector<Point> linea = linea_dritta(A, B);
    Point ultimo_libero = A;
    for (size_t i = 1; i < linea.size(); ++i) {
        if (!griglia.is_free(linea[i])) return ultimo_libero;
        ultimo_libero = linea[i];
    }
    return B;
}

//dal punto(P) c'è linea dritta libera verso arrivo(B)
bool vede_punto_di_arrivo(Point P, Point B, const Grid &griglia) {
    std::vector<Point> linea = linea_dritta(P, B);
    for (size_t i = 1; i < linea.size(); ++i)
        if (!griglia.is_free(linea[i])) return false;
    return true;
}

//dal punto(P) c'è linea dritta libera per 1 bool in direzione dell'arrivo(B)(non entra in loop)
bool vede_spazio_in_direzione_arrivo(Point P, Point B, const Grid &griglia, int id_ostacolo) {
    std::vector<Point> linea = linea_dritta(P, B);
    for (size_t i = 1; i < linea.size(); ++i) {
        Point p = linea[i];
        if (!griglia.is_free(p)) {
            int id_colpito = griglia.get_id_ostacolo(p);
            if (id_colpito == id_ostacolo) return false;
            else return true;
        }
        if (p == B) return true;
    }
    return true;
}

Border_exit segui_il_bordo_monodirezione(Point partenza, Point destinazione, const Grid& griglia,
                                        int id_ostacolo, senso_di_percorrenza direzione) {
    Border_exit risultato;
    const int MAX_PASSI = 2000;

    Point corrente = partenza;
    Path percorso;
    percorso.points.push_back(corrente);

    int dir_precedente = -1;

//gestisce casi limite con contatto in diagonale su ostacolo ////////////////////////////////////////////////////////////in teoria
    //guarda da che lato è l'ostacolo e manda o in senso orario o antiorario
    for (int d = 0; d < 4; ++d) {
        Point vicino(partenza.x + DX[d], partenza.y + DY[d]);
        if (!griglia.is_free(vicino) && griglia.get_id_ostacolo(vicino) == id_ostacolo) {
            if (direzione == orario) dir_precedente = (d + 3) % 4;
            else                    dir_precedente = (d + 1) % 4;
            break;
        }
    }
//se non si sposta dir => i vicini sono liberi,  ricontrolla e va a distanza 2 
    if (dir_precedente == -1) {
        bool spostato = false;
        for (int d = 0; d < 4; ++d) {
            Point vicino(partenza.x + DX[d], partenza.y + DY[d]);
            if (!griglia.is_free(vicino)) continue;
            for (int k = 0; k < 4; ++k) {
                Point adiacente(vicino.x + DX[k], vicino.y + DY[k]);
                if (!griglia.is_free(adiacente) && griglia.get_id_ostacolo(adiacente) == id_ostacolo) {
                    corrente = vicino;
                    percorso.points.push_back(corrente);
                    if (direzione == orario) dir_precedente = (k + 3) % 4;
                    else                    dir_precedente = (k + 1) % 4;
                    spostato = true;
                    break;
                }
            }
            if (spostato) break;
        }
        if (!spostato) return risultato;
    }

    std::set<Point> visitati;
    visitati.insert(partenza);
    if (!(corrente == partenza)) visitati.insert(corrente);
    int passi = 0;
//trova uscite, seguendo il bordo
    while (passi < MAX_PASSI) {
        if (vede_spazio_in_direzione_arrivo(corrente, destinazione, griglia, id_ostacolo)) {
            risultato.exit = corrente;
            risultato.bordo = percorso;
            risultato.direzione = direzione;
            return risultato;
        }

        int priorita[4]; //controlla prima verso bordo, poi dritto, poi opposto, poi torna indietro;   inserisce in prorita che poi viene usato dopo per decidere cosa porvare prima
        if (direzione == orario) {
            priorita[0] = (dir_precedente + 1) % 4;
            priorita[1] = dir_precedente;
            priorita[2] = (dir_precedente + 3) % 4;
            priorita[3] = (dir_precedente + 2) % 4;
        } else {
            priorita[0] = (dir_precedente + 3) % 4;
            priorita[1] = dir_precedente;
            priorita[2] = (dir_precedente + 1) % 4;
            priorita[3] = (dir_precedente + 2) % 4;
        }

        bool spostato = false;//va nel successivo
        for (int i = 0; i < 4; ++i) {
            int tdir = priorita[i];
            Point prossimo(corrente.x + DX[tdir], corrente.y + DY[tdir]);
            if (!griglia.is_free(prossimo)) continue;

            bool tocca_ostacolo = false;
            for (int k = 0; k < 8; ++k) {
                static const int DX8[8] = {0,1,1,1,0,-1,-1,-1};
                static const int DY8[8] = {-1,-1,0,1,1,1,0,-1};
                Point adiacente(prossimo.x + DX8[k], prossimo.y + DY8[k]);
                if (!griglia.is_free(adiacente) && griglia.get_id_ostacolo(adiacente) == id_ostacolo) {
                    tocca_ostacolo = true;
                    break;
                }
            }
            if (!tocca_ostacolo) continue;
            if (visitati.find(prossimo) != visitati.end()) continue;

            corrente = prossimo;
            percorso.points.push_back(corrente);
            visitati.insert(corrente);
            dir_precedente = tdir;
            spostato = true;
            break;
        }
        if (!spostato) break;
        ++passi;
    }

    return Border_exit(); 
}

senso_di_percorrenza direzione_opposta(senso_di_percorrenza d) {
    return (d == orario) ? antiorario : orario;
}

void costruisci_percorsi(Point corrente, Point destinazione, Point origine,
                         const Grid& griglia,
                         std::set<int> ostacoli_visitati,
                         std::vector<Point> waypoints_correnti,
                         std::vector<std::pair<int,int>> detours_correnti,
                         int costo_corrente,
                         std::vector<Path>& percorsi_output,
                         int profondita) {

    const int MAX_PROFONDITA = 6;
    if (profondita > MAX_PROFONDITA) return;

//se no ostacoli
    if (vede_punto_di_arrivo(corrente, destinazione, griglia)) {
        waypoints_correnti.push_back(destinazione);
        costo_corrente += static_cast<int>(linea_dritta(corrente, destinazione).size());
        Path p;
        p.points = waypoints_correnti;
        p.cost = costo_corrente;
        p.detours = detours_correnti;
        percorsi_output.push_back(p);
        return;
    }

    Point primo_colpito = primo_ostacolo_sulla_linea(corrente, destinazione, griglia);
    if (!primo_colpito.e_valido()) return;

    int id_ost = griglia.get_id_ostacolo(primo_colpito);
    if (ostacoli_visitati.count(id_ost)) return;   

    Point P0 = ultimo_punto_prima_del_primo_ostacolo(corrente, destinazione, griglia);

    bool adiacente = false;
    for (int d = 0; d < 8; ++d) {
        static const int DX8[8] = {0,1,1,1,0,-1,-1,-1};
        static const int DY8[8] = {-1,-1,0,1,1,1,0,-1};
        Point nb(P0.x + DX8[d], P0.y + DY8[d]);
        if (!griglia.is_free(nb) && griglia.get_id_ostacolo(nb) == id_ost) {
            adiacente = true; break;
        }
    }
    if (!adiacente) {
        for (int d = 0; d < 4; ++d) {
            Point nb(primo_colpito.x + DX[d], primo_colpito.y + DY[d]);
            if (griglia.is_free(nb)) { P0 = nb; break; }
        }
    }


    std::vector<Point> tratto_P0 = linea_dritta(corrente, P0);////////////////////////////////////////////////////////////?????????????????????????????
    if (!tratto_P0.empty() && tratto_P0.back() == P0) tratto_P0.pop_back();
    int costo_verso_P0 = static_cast<int>(tratto_P0.size());


    for (int d = 0; d < 2; ++d) {
        senso_di_percorrenza dir = (d == 0) ? orario : antiorario;
        senso_di_percorrenza dir_inversa = direzione_opposta(dir);


        Border_exit uscita_dest = segui_il_bordo_monodirezione(P0, destinazione, griglia, id_ost, dir);
        if (!uscita_dest.exit.e_valido()) continue;
        Point C = uscita_dest.exit;
        int costo_bordo_C = static_cast<int>(uscita_dest.bordo.points.size()) - 1;

        Border_exit uscita_orig = segui_il_bordo_monodirezione(C, origine, griglia, id_ost, dir_inversa);///////////////////////////////////////////non funziona devo effettivamentr fare come dicevo a noce quindi applicare tutto costruisci percorsi all'indietros
        if (!uscita_orig.exit.e_valido()) continue;
        Point P = uscita_orig.exit;
        int costo_bordo_P = static_cast<int>(uscita_orig.bordo.points.size()) - 1;


        std::set<int> nuovi_visitati = ostacoli_visitati;
        nuovi_visitati.insert(id_ost);


        std::vector<Point> nuovi_waypoints = waypoints_correnti;
        if (nuovi_waypoints.empty()) nuovi_waypoints.push_back(corrente);
        if (!(P == corrente) && std::find(nuovi_waypoints.begin(), nuovi_waypoints.end(), P) == nuovi_waypoints.end())
            nuovi_waypoints.push_back(P);
        if (!(C == P) && std::find(nuovi_waypoints.begin(), nuovi_waypoints.end(), C) == nuovi_waypoints.end())
            nuovi_waypoints.push_back(C);


        int nuovo_costo = costo_corrente + costo_verso_P0 + costo_bordo_C + costo_bordo_P;


        std::vector<std::pair<int,int>> nuovi_detours = detours_correnti;
        nuovi_detours.push_back({id_ost, d});

        costruisci_percorsi(C, destinazione, origine, griglia, nuovi_visitati,
                            nuovi_waypoints, nuovi_detours, nuovo_costo,
                            percorsi_output, profondita + 1);
    }
}

//se è dritto fa, se no costruisvi_percorsi, ordina per costo da min a max
std::vector<Path> trova_paths(Point A, Point B, const Grid& griglia,
                              std::set<int>& ostacoli_visitati, int depth) {
    std::vector<Path> risultati;
    if (vede_punto_di_arrivo(A, B, griglia)) {
        Path diretto;
        diretto.points = {A, B};
        diretto.cost = static_cast<int>(linea_dritta(A, B).size());
        return {diretto};
    }

    costruisci_percorsi(A, B, A, griglia, {}, {}, {}, 0, risultati, 0);

    std::sort(risultati.begin(), risultati.end(),
              [](const Path& a, const Path& b) { return a.points < b.points; });
    risultati.erase(std::unique(risultati.begin(), risultati.end(),
                                [](const Path& a, const Path& b) { return a.points == b.points; }),
                    risultati.end());

    return risultati;
}

std::vector<Path> filtra_paths(std::vector<Path>& percorsi) {
    if (percorsi.empty()) return {};
    int costo_minimo = percorsi[0].cost;
    for (const auto& p : percorsi) if (p.cost < costo_minimo) costo_minimo = p.cost;
    int costo_massimo = static_cast<int>(costo_minimo * 1.2);

    std::vector<Path> filtrati;
    std::set<std::vector<std::pair<int, int>>> deviazioni_viste;
    for (const Path& p : percorsi) {
        if (p.cost > costo_massimo) continue;
        if (deviazioni_viste.find(p.detours) == deviazioni_viste.end()) {
            deviazioni_viste.insert(p.detours);
            filtrati.push_back(p);
        }
    }
    return filtrati;
}

//copia tuuto il testo, trova l'ultimo oggetto e sotto mette linee
void printa_path(const Path &p, int larghezza, int altezza,
                 const std::vector<std::vector<bool>> &griglia) {
    std::cout << "Percorso (costo: " << p.cost << "), waypoint: ";
    for (size_t i = 0; i < p.points.size(); ++i) {
        std::cout << "(" << p.points[i].x << "," << p.points[i].y << ")";
        if (i < p.points.size()-1) std::cout << " -> ";
    }
    std::cout << "\n";

    std::vector<std::vector<char>> visuale(altezza, std::vector<char>(larghezza, '.'));
    for (int y = 0; y < altezza; ++y)
        for (int x = 0; x < larghezza; ++x)
            if (!griglia[y][x]) visuale[y][x] = '#';

    for (size_t i = 0; i < p.points.size()-1; ++i) {
        std::vector<Point> linea = linea_dritta(p.points[i], p.points[i+1]);
        for (const Point& pt : linea)
            if (pt.x >= 0 && pt.x < larghezza && pt.y >= 0 && pt.y < altezza && visuale[pt.y][pt.x] != '#')
                visuale[pt.y][pt.x] = '*';
    }

    if (!p.points.empty()) {
        Point A = p.points.front();
        Point B = p.points.back();
        if (A.x >= 0 && A.x < larghezza && A.y >= 0 && A.y < altezza) visuale[A.y][A.x] = 'A';
        if (B.x >= 0 && B.x < larghezza && B.y >= 0 && B.y < altezza) visuale[B.y][B.x] = 'B';
    }

    for (int y = 0; y < altezza; ++y) {
        for (int x = 0; x < larghezza; ++x) std::cout << visuale[y][x];
        std::cout << "\n";
    }
    std::cout << "\n";
}