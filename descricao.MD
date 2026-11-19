# Ecos de Valdoria (Echoes of Valdoria) — Descrição e combinados de trabalho

Resumo da conversa de alinhamento, para o Claude Code continuar de onde paramos.

## 1. Contexto
- O projeto é do filho do usuário, que vai ajudar nele. O usuário revezará o trabalho com o filho, para ele não precisar passar mais de 16h no computador.
- Repositório no GitHub: `iAlencari/-Echoes-of-Valdoria`.
- Estado do repositório na hora da análise: **só o arquivo `LICENSE` (GNU GPL v3)** e um único commit ("Initial commit", 17/11/2026). Branches: `main` e uma branch de sessão idêntica à `main`.
- O código real **não está no GitHub**. Fica no computador (Windows), em duas pastas, uma delas `C:\Dev`. Nelas também ficam os arquivos de instruções do Claude dele e os logs de mudanças do Claude e do opencode.
- Pelo nome, parece um jogo ou história de fantasia, mas **isso ainda não foi confirmado**: nenhum arquivo do projeto foi lido.

## 2. Ferramentas e fluxo atual do filho
- Usa o **Claude Code** e o **opencode**.
- Os logs são mantidos sempre atualizados para explicar o que cada ferramenta mudou para a outra.
- Existem arquivos de instruções dele para o Claude (por exemplo `CLAUDE.md`, `AGENTS.md` ou pasta `.claude/`). Ele autorizou a leitura deles: ficam no computador e não têm informações pessoais.

## 3. Regras de privacidade (valem sempre)
- O Claude Code é só para o projeto. Assuntos pessoais ou importantes vão no **chat normal** do Claude, que fica salvo na nuvem, na conta de cada pessoa.
- Ler **somente**: as duas pastas do projeto, os arquivos de instruções e os logs de mudanças.
- **Não abrir** histórico de conversas do Claude Code ou do opencode (por exemplo `~/.claude/projects` e `~/.local/share/opencode`), nem pastas pessoais. Se algum log estiver misturado com histórico de conversas, **perguntar antes**.
- Não abrir e-mails nem usar o conector do Gmail, a menos que o usuário peça.
- Cada pessoa usa a sua própria conta do Claude, para os chats não se misturarem.
- Não colocar senhas, chaves de API ou arquivos pessoais no repositório. Ele parece ser público.

## 4. Como trabalhar (combinado)
1. **Primeiro, só leitura.** Ler instruções e logs, entender o projeto e fazer um resumo: o que é, como está organizado e em que ponto está.
2. **Não alterar, mover ou apagar nada** sem o aval do usuário e do filho. As decisões de rumo do jogo são dele.
3. **Registrar no padrão dele:** mesmo arquivo de log, mesmo formato, mesmo nível de detalhe.
4. **Todo fim de sessão deixa um registro:** o que foi feito, o que foi testado, o que ficou pendente e as dúvidas que o filho precisa decidir.
5. Se o projeto usar Git, **um commit por mudança**, com descrição clara, para ser fácil revisar e desfazer.

## 5. Problema de acesso encontrado
- A sessão em que isso foi conversado rodava **na nuvem** (Linux), não no computador. Não existe disco `C:` lá.
- A pasta `Dev` aparece na lista do Remote Control, mas o computador **não a oferece para sessões na nuvem**. Causas prováveis: app Claude no Windows desatualizado ou recurso ainda não liberado para a conta.
- **Solução:** abrir uma sessão **local** no app desktop (aba Code → local → pasta `C:\Dev`). Assim o Claude lê os arquivos direto do disco.
- Alternativa: o filho envia as duas pastas para o GitHub uma única vez. Antes, conferir se não há segredos e, se preferir, deixar o repositório privado.

## 6. Próximo passo
Em uma sessão **local** em `C:\Dev` (e na segunda pasta, quando o usuário disser qual é):
1. Ler os arquivos de instruções e os logs de mudanças, em modo só leitura.
2. Fazer o resumo do projeto.
3. Perguntar ao usuário e ao filho o que fazer em seguida, antes de mexer em qualquer arquivo.
