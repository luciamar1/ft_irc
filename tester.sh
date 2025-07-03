#!/bin/bash

# =========================================
# TESTER MEJORADO PARA SERVIDOR IRC
# =========================================
# Características:
# 1. Pruebas organizadas por módulos
# 2. Verificación automática de resultados
# 3. Informe detallado de errores
# 4. Soporte para múltiples clientes simultáneos
# 5. Limpieza automática de procesos

# Configuración
PORT=6667
PASSWORD="testpassword"
SERVER="./ircserv"
NC="nc -C"  # Usar -C para enviar retornos de carro
LOG_DIR="test_logs"
mkdir -p $LOG_DIR

# Iniciar servidor
echo "Iniciando servidor en puerto $PORT..."
$SERVER $PORT $PASSWORD > $LOG_DIR/server.log 2>&1 &
SERVER_PID=$!
sleep 2  # Esperar que el servidor se inicie

# Función para enviar comandos y verificar respuestas
test_command() {
    local test_name="$1"
    local client_id="$2"
    local commands="$3"
    local expected="$4"
    local log_file="$LOG_DIR/client_${client_id}.log"
    
    echo "Ejecutando prueba: $test_name"
    echo -e "$commands" | $NC localhost $PORT > "$log_file" 2>&1 &
    sleep 1
    
    if grep -q "$expected" "$log_file"; then
        echo "✅ $test_name - PASADO"
    else
        echo "❌ $test_name - FALLADO"
        echo "--- Comandos enviados: ---"
        echo -e "$commands"
        echo "--- Resultado esperado: ---"
        echo "$expected"
        echo "--- Respuesta del servidor: ---"
        cat "$log_file"
        echo "----------------------------"
    fi
}

# Función para pruebas multi-cliente
multi_client_test() {
    local test_name="$1"
    local clients=("${!2}")
    local commands=("${!3}")
    local expected=("${!4}")
    
    echo "Ejecutando prueba multi-cliente: $test_name"
    pids=()
    
    # Ejecutar todos los clientes
    for i in "${!clients[@]}"; do
        echo -e "${commands[$i]}" | $NC localhost $PORT > "$LOG_DIR/${test_name}_client${clients[$i]}.log" 2>&1 &
        pids+=($!)
    done
    
    # Esperar que todos terminen
    sleep 3
    
    # Verificar resultados
    all_passed=true
    for i in "${!clients[@]}"; do
        if ! grep -q "${expected[$i]}" "$LOG_DIR/${test_name}_client${clients[$i]}.log"; then
            echo "❌ $test_name (Cliente ${clients[$i]}) - FALLADO"
            echo "--- Comandos: ---"
            echo -e "${commands[$i]}"
            echo "--- Esperado: ---"
            echo "${expected[$i]}"
            echo "--- Obtenido: ---"
            cat "$LOG_DIR/${test_name}_client${clients[$i]}.log"
            echo "-----------------"
            all_passed=false
        fi
    done
    
    if $all_passed; then
        echo "✅ $test_name - PASADO"
    fi
}

# =========================================
# PRUEBAS DE CONEXIÓN Y AUTENTICACIÓN
# =========================================

test_command "Autenticación exitosa" 1 \
    "PASS $PASSWORD\nNICK user1\nUSER user1 0 * :Test User\n" \
    "You have successfully authenticated"

test_command "Contraseña incorrecta" 2 \
    "PASS wrongpass\n" \
    "Incorrect password"

test_command "Registro incompleto" 3 \
    "PASS $PASSWORD\nNICK user3\n" \
    "not registered"

# =========================================
# PRUEBAS DE CANALES
# =========================================

# Cliente 1 crea canal
test_command "Creación de canal" 1 \
    "PASS $PASSWORD\nNICK user1\nUSER user1 0 * :Test User\nJOIN #test\n" \
    "JOIN :#test"

# Cliente 2 se une al canal
test_command "Unión a canal existente" 2 \
    "PASS $PASSWORD\nNICK user2\nUSER user2 0 * :Test User\nJOIN #test\n" \
    "JOIN :#test"

# Prueba de mensajes
test_command "Mensaje a canal" 1 \
    "PRIVMSG #test :Hola a todos\n" \
    "PRIVMSG #test :Hola a todos"

# =========================================
# PRUEBAS DE COMANDOS ESPECÍFICOS
# =========================================

# KICK
test_command "Expulsión de usuario" 1 \
    "KICK #test user2 :Razón de prueba\n" \
    "KICK #test user2 :Razón de prueba"

# TOPIC
test_command "Establecer tema" 1 \
    "TOPIC #test :Tema de prueba con espacios\n" \
    "TOPIC #test :Tema de prueba con espacios"

# INVITE
test_command "Invitación a canal" 1 \
    "INVITE user3 #test\n" \
    "INVITE user3 :#test"

# =========================================
# PRUEBAS MULTI-CLIENTE
# =========================================

# Preparar datos para prueba multi-cliente
clients=(4 5)
commands=(
    "PASS $PASSWORD\nNICK user4\nUSER user4 0 * :User 4\nJOIN #multi\nPRIVMSG #multi :Hola desde user4\n"
    "PASS $PASSWORD\nNICK user5\nUSER user5 0 * :User 5\nJOIN #multi\n"
)
expected=(
    "PRIVMSG #multi :Hola desde user4"
    "PRIVMSG #multi :Hola desde user4"
)

multi_client_test "Comunicación multi-cliente" clients[@] commands[@] expected[@]

# =========================================
# PRUEBAS DE MODO INVITACIÓN (+i)
# =========================================

# Configurar canal +i
test_command "Configurar canal +i" 1 \
    "MODE #test +i\n" \
    "MODE #test +i"

# Intentar unirse sin invitación
test_command "Unión a canal +i sin invitación" 6 \
    "PASS $PASSWORD\nNICK user6\nUSER user6 0 * :User 6\nJOIN #test\n" \
    "Cannot join channel"

# Invitar y unirse
test_command "Invitación a canal +i" 1 \
    "INVITE user6 #test\n" \
    "INVITE user6 :#test"

test_command "Unión con invitación" 6 \
    "JOIN #test\n" \
    "JOIN :#test"

# =========================================
# LIMPIEZA Y FINALIZACIÓN
# =========================================

echo "Deteniendo servidor..."
kill $SERVER_PID
wait $SERVER_PID 2>/dev/null

echo "Limpiando logs..."
rm -rf $LOG_DIR

echo "Todas las pruebas completadas!"