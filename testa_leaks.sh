#!/bin/bash

# Diretórios
DIR_TESTES="Testes"
DIR_ENTRADA="Entrada"

# Cores para o output
VERDE='\e[32m'
VERMELHO='\e[31m'
AMARELO='\e[33m'
RESET='\e[0m'

# Verifica se a pasta de testes existe
if [ ! -d "$DIR_TESTES" ]; then
    echo -e "${VERMELHO}Erro: Diretório '$DIR_TESTES' não encontrado.${RESET}"
    exit 1
fi

echo "⚙️  Compilando o projeto..."
make all

# Copia os arquivos de entrada para o diretório atual
if [ -d "$DIR_ENTRADA" ]; then
    echo "📂 Trazendo arquivos base da pasta '$DIR_ENTRADA' para a raiz..."
    cp -r "$DIR_ENTRADA"/* . 2>/dev/null
fi

echo ""
echo "🔍 Iniciando bateria de testes com Valgrind (make leaks)..."
echo "--------------------------------------------------------"

erros=0
total=0

# Itera sobre todos os arquivos .in na pasta Testes
for teste in "$DIR_TESTES"/*.in; do
    [ -e "$teste" ] || continue

    total=$((total + 1))
    nome=$(basename "$teste")
    echo -n "Testando $nome... "

    # LC_ALL=C força o output do Valgrind em inglês para o grep não falhar
    LC_ALL=C make leaks < "$teste" > tmp_out.txt 2> tmp_err.txt

    # Checa 0 errors E (Todos os blocos liberados OU 0 bytes definitivamente perdidos)
    if grep -q "ERROR SUMMARY: 0 errors" tmp_err.txt && \
       (grep -q "All heap blocks were freed" tmp_err.txt || grep -q "definitely lost: 0 bytes" tmp_err.txt); then
        echo -e "${VERDE}✅ OK (Sem Leaks)${RESET}"
    else
        echo -e "${VERMELHO}❌ FALHOU (Leaks detectados)${RESET}"
        erros=$((erros + 1))
        
        # Mostra o resumo do problema 
        grep "definitely lost:" tmp_err.txt | sed 's/==[0-9]*==/   /g'
        grep "ERROR SUMMARY:" tmp_err.txt | sed 's/==[0-9]*==/   /g'
    fi
done

echo "--------------------------------------------------------"
if [ $erros -eq 0 ] && [ $total -gt 0 ]; then
    echo -e "${VERDE}🎉 Sucesso absoluto! 0 vazamentos em $total testes!${RESET}"
else
    echo -e "${AMARELO}⚠️  Atenção: Falhou em $erros de $total teste(s). Verifique os relatórios detalhados.${RESET}"
fi

echo ""
echo "🧹 Limpando arquivos temporários..."
rm -f tmp_out.txt tmp_err.txt

# Remove os arquivos da pasta Entrada que foram copiados para a raiz
if [ -d "$DIR_ENTRADA" ]; then
    find "$DIR_ENTRADA" -maxdepth 1 -type f -exec basename {} \; | while read -r arquivo; do
        rm -f "$arquivo"
    done
fi