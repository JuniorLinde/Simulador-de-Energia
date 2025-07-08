import time
import os
from datetime import datetime
import socket

HOST = '127.0.0.1'  
PORT = 12345   

def limpar_tela():
    """Limpa o console."""
    os.system("cls" if os.name == "nt" else "clear")

def obter_dados_do_servidor_cpp(hora, segundo):
    """
    Conecta ao servidor C++, envia a hora/segundo e recebe os dados de produção.
    Retorna um dicionário com os dados ou None em caso de erro.
    """
    try:
        with socket.socket(socket.AF_INET, socket.SOCK_STREAM) as s:
            s.connect((HOST, PORT))
            
            mensagem_para_cpp = f"hora:{hora},segundo:{segundo}"
            s.sendall(mensagem_para_cpp.encode('utf-8'))
            
            dados_brutos = s.recv(4096).decode('utf-8') # Buffer de 4KB
            
            return parse_dados_cpp(dados_brutos)
    except ConnectionRefusedError:
        print(f"Erro: Não foi possível conectar ao servidor C++ em {HOST}:{PORT}.")
        print("Certifique-se de que o programa C++ está rodando no Visual Studio.")
        return None
    except Exception as e:
        print(f"Erro de comunicação com o servidor C++: {e}")
        return None

def parse_dados_cpp(dados_brutos):
    """
    Analisa a string de dados recebida do servidor C++ e a converte em um dicionário.
    Formato esperado: "fonte1:producao,fator;fonte2:producao,fator;...;total:valor"
    """
    dados_processados = {}
    partes = dados_brutos.split(';')

    for parte in partes:
        if ':' in parte:
            nome, valores = parte.split(':', 1)
            if nome == 'total':
                dados_processados['total'] = float(valores)
            else:
                prod_str, fator_str = valores.split(',')
                dados_processados[nome] = {
                    "producao": float(prod_str),
                    "fator": float(fator_str)
                }
    return dados_processados

if __name__ == "__main__":
    while True:
        agora = datetime.now()
        hora = agora.hour
        segundo = agora.second
        
        dados = obter_dados_do_servidor_cpp(hora, segundo)
        
        limpar_tela()

        if dados:
            total_producao = dados.get('total', 0.0)
            
            print(f"=== PRODUÇÃO INSTANTÂNEA - {agora.strftime('%H:%M:%S')} ===")
            print(f"{'Fonte':10} | {'Produção (kW)':>15} | {'Fator Capacidade':>18}")
            print("-" * 50)
            
            
            fontes_para_exibir = {k: v for k, v in dados.items() if k != 'total'}

            for fonte, info in fontes_para_exibir.items():
                print(f"{fonte.capitalize():10} | {info['producao']:15.2f} | {info['fator']:18.3f}")
            print("-" * 50)

            
            print(f"\nResumo da produção total no instante {agora.strftime('%H:%M:%S')}:")
            print(f" - Potência total gerada: {total_producao:,.2f} kW")
            print(" - Observação: Valores atualizados a cada 5 segundos.\n")
        else:
            print(f"Não foi possível obter dados do servidor C++ no instante {agora.strftime('%H:%M:%S')}.")
            print("Verifique se o servidor C++ está em execução e acessível.")
            
        time.sleep(30) 
