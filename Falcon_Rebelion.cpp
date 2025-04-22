#include <iostream>
#include <ev3dev.h>
#include <unistd.h>
#include <thread>

using namespace ev3dev;
using namespace std;

// Declaração dos sensores e motores
color_sensor sensor_cor_esq(INPUT_1);
color_sensor sensor_cor_dir(INPUT_2);
infrared_sensor sensor_ir_esq(INPUT_3);
infrared_sensor sensor_ir_dir(INPUT_4);
large_motor motor_esq(OUTPUT_B);
large_motor motor_dir(OUTPUT_C);

// Variáveis de configuração
const int VELOCIDADE_BASE = 50;
const int LIMIAR_COR = 50; // Ajustar conforme necessário
const int DISTANCIA_MINIMA = 20; // cm

// Função para verificar se todos os dispositivos estão conectados
bool verificar_conexoes() {
    if (!sensor_cor_esq.connected() || !sensor_cor_dir.connected() ||
        !sensor_ir_esq.connected() || !sensor_ir_dir.connected() ||
        !motor_esq.connected() || !motor_dir.connected()) {
        cerr << "Erro: Um ou mais dispositivos não estão conectados!" << endl;
        return false;
    }
    return true;
}

// Função para seguir linha usando os sensores de cor
void seguir_linha() {
    while (true) {
        // Ler valores dos sensores de cor (intensidade de luz refletida)
        int valor_esq = sensor_cor_esq.reflected_light_intensity();
        int valor_dir = sensor_cor_dir.reflected_light_intensity();
        
        // Diferença entre os sensores
        int diferenca = valor_esq - valor_dir;
        
        // Ajustar motores baseado na diferença
        if (abs(diferenca) < LIMIAR_COR) {
            // Seguir em frente
            motor_esq.set_speed_sp(VELOCIDADE_BASE);
            motor_dir.set_speed_sp(VELOCIDADE_BASE);
        } else if (diferenca > 0) {
            // Virar para direita
            motor_esq.set_speed_sp(VELOCIDADE_BASE);
            motor_dir.set_speed_sp(VELOCIDADE_BASE * 0.5);
        } else {
            // Virar para esquerda
            motor_esq.set_speed_sp(VELOCIDADE_BASE * 0.5);
            motor_dir.set_speed_sp(VELOCIDADE_BASE);
        }
        
        // Ativar motores
        motor_esq.run_forever();
        motor_dir.run_forever();
        
        // Pequena pausa para evitar sobrecarga
        usleep(10000);
    }
}

// Função para evitar obstáculos usando sensores IR
void evitar_obstaculos() {
    while (true) {
        // Ler distâncias dos sensores IR (em cm)
        float distancia_esq = sensor_ir_esq.distance();
        float distancia_dir = sensor_ir_dir.distance();
        
        // Verificar obstáculos
        if (distancia_esq < DISTANCIA_MINIMA || distancia_dir < DISTANCIA_MINIMA) {
            // Parar motores
            motor_esq.stop();
            motor_dir.stop();
            
            // Dar ré e virar
            motor_esq.set_speed_sp(-VELOCIDADE_BASE);
            motor_dir.set_speed_sp(-VELOCIDADE_BASE);
            motor_esq.run_timed(500000); // 0.5 segundos
            motor_dir.run_timed(500000);
            
            // Virar para o lado oposto ao obstáculo
            if (distancia_esq < distancia_dir) {
                motor_esq.set_speed_sp(VELOCIDADE_BASE);
                motor_dir.set_speed_sp(-VELOCIDADE_BASE);
            } else {
                motor_esq.set_speed_sp(-VELOCIDADE_BASE);
                motor_dir.set_speed_sp(VELOCIDADE_BASE);
            }
            motor_esq.run_timed(1000000); // 1 segundo
            motor_dir.run_timed(1000000);
        }
        
        // Pequena pausa
        usleep(100000);
    }
}

int main() {
    // Verificar conexões dos dispositivos
    if (!verificar_conexoes()) {
        return 1;
    }
    
    // Configurar modo dos sensores
    sensor_cor_esq.set_mode(color_sensor::mode_reflect);
    sensor_cor_dir.set_mode(color_sensor::mode_reflect);
    sensor_ir_esq.set_mode(infrared_sensor::mode_dist_cm);
    sensor_ir_dir.set_mode(infrared_sensor::mode_dist_cm);
    
    // Iniciar threads para as funções principais
    thread thread_seguir_linha(seguir_linha);
    thread thread_evitar_obstaculos(evitar_obstaculos);
    
    // Manter o programa rodando
    while (true) {
        sleep(1);
    }
    
    // Nunca deve chegar aqui
    thread_seguir_linha.join();
    thread_evitar_obstaculos.join();
    
    return 0;
}