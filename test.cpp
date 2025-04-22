#include <ev3dev.h>
#include <iostream>
#include <unistd.h>
#include <thread>
#include <vector>
#include <algorithm>
#include <numeric>

using namespace ev3dev;
using namespace std;

// Configurações de hardware
color_sensor sensor_cor_esq(INPUT_1);
color_sensor sensor_cor_dir(INPUT_2);
large_motor motor_esq(OUTPUT_B);
large_motor motor_dir(OUTPUT_C);

// Constantes ajustáveis
const int VELOCIDADE_BASE = 40;
const int TEMPO_GIRO = 800000;
const int TAMANHO_JANELA_FILTRO = 5;
const int INTERVALO_CALIBRACAO = 10000; // μs

// Variáveis de estado
enum ModoRobo { SEGUIR_LINHA, RESGATE };
ModoRobo modo_atual = SEGUIR_LINHA;

// Sistema de calibração
struct {
    int min_esq = 100, max_esq = 0;
    int min_dir = 100, max_dir = 0;
    bool calibrado = false;
} calibracao;

// Filtros de média móvel
vector<int> historico_esq(TAMANHO_JANELA_FILTRO, 0);
vector<int> historico_dir(TAMANHO_JANELA_FILTRO, 0);

// Protótipos de funções
bool verificar_conexoes();
void calibrar_sensores();
int ler_sensor_filtrado(color_sensor& sensor, vector<int>& historico);
void seguir_linha_adaptativo();
void detectar_cores_obr();

// Função principal
int main() {
    if (!verificar_conexoes()) return 1;

    // Configuração inicial
    sensor_cor_esq.set_mode(color_sensor::mode_reflect);
    sensor_cor_dir.set_mode(color_sensor::mode_reflect);
    
    // Thread para calibração contínua
    thread thread_calibracao([](){
        while (true) {
            calibrar_sensores();
            usleep(INTERVALO_CALIBRACAO);
        }
    });

    // Thread para detecção de cores
    thread thread_detectar(detectar_cores_obr);

    // Loop principal
    while (true) {
        if (modo_atual == SEGUIR_LINHA) {
            seguir_linha_adaptativo();
        }
        usleep(10000);
    }

    thread_calibracao.join();
    thread_detectar.join();
    return 0;
}

bool verificar_conexoes() {
    if (!sensor_cor_esq.connected() || !sensor_cor_dir.connected() ||
        !motor_esq.connected() || !motor_dir.connected()) {
        cerr << "Erro: Dispositivos não conectados!" << endl;
        return false;
    }
    return true;
}

void calibrar_sensores() {
    int val_esq = sensor_cor_esq.reflected_light_intensity();
    int val_dir = sensor_cor_dir.reflected_light_intensity();

    calibracao.min_esq = min(calibracao.min_esq, val_esq);
    calibracao.max_esq = max(calibracao.max_esq, val_esq);
    calibracao.min_dir = min(calibracao.min_dir, val_dir);
    calibracao.max_dir = max(calibracao.max_dir, val_dir);

    if (!calibracao.calibrado && calibracao.max_esq > 0 && calibracao.max_dir > 0) {
        calibracao.calibrado = true;
        cout << "Calibração completa!" << endl;
    }
}

int ler_sensor_filtrado(color_sensor& sensor, vector<int>& historico) {
    rotate(historico.begin(), historico.begin()+1, historico.end());
    historico.back() = sensor.reflected_light_intensity();
    return accumulate(historico.begin(), historico.end(), 0) / historico.size();
}

int normalizar_valor(int valor, int min_val, int max_val) {
    if (!calibracao.calibrado) return valor;
    return 100 * (valor - min_val) / max(1, (max_val - min_val));
}

void seguir_linha_adaptativo() {
    int val_esq = normalizar_valor(
        ler_sensor_filtrado(sensor_cor_esq, historico_esq),
        calibracao.min_esq, calibracao.max_esq);
    
    int val_dir = normalizar_valor(
        ler_sensor_filtrado(sensor_cor_dir, historico_dir),
        calibracao.min_dir, calibracao.max_dir);

    int diferenca = val_esq - val_dir;
    int limiar = 20 + (100 - max(val_esq, val_dir)) / 5;

    if (abs(diferenca) < limiar) {
        motor_esq.set_speed_sp(VELOCIDADE_BASE);
        motor_dir.set_speed_sp(VELOCIDADE_BASE);
    } else if (diferenca > 0) {
        motor_esq.set_speed_sp(VELOCIDADE_BASE * 0.7);
        motor_dir.set_speed_sp(VELOCIDADE_BASE * 1.3);
    } else {
        motor_esq.set_speed_sp(VELOCIDADE_BASE * 1.3);
        motor_dir.set_speed_sp(VELOCIDADE_BASE * 0.7);
    }

    motor_esq.run_forever();
    motor_dir.run_forever();
}

void detectar_cores_obr() {
    sensor_cor_esq.set_mode(color_sensor::mode_rgb_raw);
    sensor_cor_dir.set_mode(color_sensor::mode_rgb_raw);

    while (true) {
        vector<int> rgb_esq = sensor_cor_esq.rgb_raw();
        vector<int> rgb_dir = sensor_cor_dir.rgb_raw();

        // Detecção de verde (baseado na dominância do componente G)
        bool verde_esq = (rgb_esq[1] > rgb_esq[0]*1.5) && (rgb_esq[1] > rgb_esq[2]*1.5);
        bool verde_dir = (rgb_dir[1] > rgb_dir[0]*1.5) && (rgb_dir[1] > rgb_dir[2]*1.5);

        // Detecção de prateado (todos componentes RGB altos e balanceados)
        bool prateado_esq = (rgb_esq[0] > 200 && rgb_esq[1] > 200 && rgb_esq[2] > 200) && 
                          (abs(rgb_esq[0]-rgb_esq[1]) < 30 && abs(rgb_esq[1]-rgb_esq[2]) < 30);
        bool prateado_dir = (rgb_dir[0] > 200 && rgb_dir[1] > 200 && rgb_dir[2] > 200) && 
                          (abs(rgb_dir[0]-rgb_dir[1]) < 30 && abs(rgb_dir[1]-rgb_dir[2]) < 30);

        // Lógica de decisão (sem vermelho)
        if (prateado_esq || prateado_dir) {
            modo_atual = RESGATE;
            motor_esq.stop();
            motor_dir.stop();
            cout << "MODO RESGATE ATIVADO!" << endl;
            sleep(2);
            modo_atual = SEGUIR_LINHA;
        } 
        else if (verde_esq && verde_dir) {
            motor_esq.set_speed_sp(-VELOCIDADE_BASE);
            motor_dir.set_speed_sp(-VELOCIDADE_BASE);
            motor_esq.run_timed(TEMPO_GIRO);
            motor_dir.run_timed(TEMPO_GIRO);
            cout << "DOIS VERDES: VOLTANDO..." << endl;
        } 
        else if (verde_esq) {
            motor_esq.set_speed_sp(-VELOCIDADE_BASE);
            motor_dir.set_speed_sp(VELOCIDADE_BASE);
            motor_esq.run_timed(TEMPO_GIRO);
            motor_dir.run_timed(TEMPO_GIRO);
            cout << "VERDE À ESQUERDA: GIRANDO..." << endl;
        } 
        else if (verde_dir) {
            motor_esq.set_speed_sp(VELOCIDADE_BASE);
            motor_dir.set_speed_sp(-VELOCIDADE_BASE);
            motor_esq.run_timed(TEMPO_GIRO);
            motor_dir.run_timed(TEMPO_GIRO);
            cout << "VERDE À DIREITA: GIRANDO..." << endl;
        }

        usleep(100000);
    }
}