# Linux Min Heap Device Driver 🐧

Este repositório contém a implementação de um **Driver de Dispositivo de Caractere (Character Device Driver)** para Linux, desenvolvido como projeto final da disciplina de **Sistemas Operacionais I**.

O objetivo do projeto é criar um módulo de kernel que gerencia uma estrutura de dados **Min Heap** (Heap de Mínimo). O driver permite que usuários insiram números inteiros desordenados e os recuperem em ordem crescente através de operações de arquivo padrão (`write` e `read`).

## 📋 Funcionalidades

- **Dispositivo de Caractere:** Cria automaticamente uma entrada em `/dev/minheap_device`.
- **Estrutura Min Heap:** Implementação nativa de uma Min Heap estática dentro do espaço do kernel.
- **Operações de I/O:**
  - `write`: Recebe uma string numérica, converte para inteiro e insere na Heap.
  - `read`: Remove a raiz da Heap (menor valor), converte para string e retorna ao usuário.
- **Concorrência Segura:** Utiliza `mutex` (exclusão mútua) para garantir que múltiplos processos não corrompam a estrutura de dados ao tentar acessar o driver simultaneamente.
- **Segurança de Memória:** Utiliza `copy_from_user` e `copy_to_user` para transferir dados entre User Space e Kernel Space de forma segura.

## 🛠️ Requisitos

- Sistema Operacional Linux (Testado no Kernel 6.12+).
- `gcc` (GNU Compiler Collection).
- `make`.
- Linux Kernel Headers instalados.

## 🚀 Como Compilar e Executar

Siga os passos abaixo para compilar o módulo e carregá-lo no kernel.

### 1. Compilação
Abra o terminal na pasta do projeto e execute:

```bash
make
````

Isso irá gerar o arquivo binário do módulo: `minheap_driver.ko`.

### 2\. Carregar o Módulo

Insira o módulo no kernel (requer permissões de superusuário):

```bash
sudo insmod minheap_driver.ko
```

Verifique se o módulo foi carregado corretamente observando os logs do kernel:

```bash
sudo dmesg | tail
```

### 3\. Permissões

O dispositivo será criado em `/dev/minheap_device`. Para facilitar os testes sem usar `sudo` para cada comando, altere as permissões:

```bash
sudo chmod 666 /dev/minheap_device
```

## 🧪 Como Testar

Você pode interagir com o driver usando comandos simples de terminal como `echo` e `cat`.

### Cenário de Teste

Vamos inserir os números **50, 10 e 30** (fora de ordem).

**Passo 1: Escrita (Inserção)**

```bash
echo "50" > /dev/minheap_device
echo "10" > /dev/minheap_device
echo "30" > /dev/minheap_device
```

**Passo 2: Leitura (Extração)**
A Min Heap deve retornar sempre o menor número disponível.

```bash
cat /dev/minheap_device
# Saída esperada: 10

cat /dev/minheap_device
# Saída esperada: 30

cat /dev/minheap_device
# Saída esperada: 50
```

## ⚙️ Detalhes Técnicos

### Estrutura do Projeto

  - `minheap_driver.c`: Código fonte contendo a lógica da Heap e as `file_operations` do Linux.
  - `Makefile`: Script de automação para compilação utilizando o kbuild do Linux.

### Compatibilidade de Kernel

Este código foi atualizado para suportar as mudanças da API do Kernel Linux a partir da versão **6.4+**, especificamente:

  - Uso da função `class_create` com apenas um argumento.
  - Renomeação de funções internas para evitar conflito com macros `swap` nativas do kernel.

## 🧹 Limpeza e Remoção

Para remover o driver da memória e limpar os arquivos compilados:

```bash
# Remove o módulo do kernel
sudo rmmod minheap_driver

# Limpa os arquivos binários da pasta
make clean
```

## ⚠️ Aviso

Este é um módulo de kernel. Erros de programação neste nível (como loops infinitos ou acesso indevido de memória) podem causar travamento do sistema operacional. Recomenda-se testar em uma Máquina Virtual.
