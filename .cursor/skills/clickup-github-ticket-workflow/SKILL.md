---
name: clickup-github-ticket-workflow
description: >-
  Fractal-Distortion — fluxo ClickUp + Git + GitHub PR por ticket (branch dedicada,
  link do PR na tarefa, estado e encerramento após merge). Usar quando o
  utilizador pedir para implementar um ticket, abrir PR, sincronizar ClickUp
  com o GitHub, ou fechar tarefa após merge. Requer MCP `user-clickup` e CLI
  `gh` autenticado no remoto do repo.
---

# ClickUp + GitHub — fluxo por ticket (Fractal-Distortion)

## Princípios

1. **Um ticket ClickUp → uma branch Git → um PR** (não misturar tarefas na mesma branch sem acordo explícito).
2. **Cada tarefa deve referenciar o PR** (comentário e/ou descrição com URL do GitHub).
3. **Estados ClickUp** só com nomes válidos na lista da tarefa (ver `reference.md` e erros da API).
4. Antes de chamar ferramentas MCP: ler o schema em `mcps/user-clickup/tools/<tool>.json` do projecto Cursor quando necessário.

## IDs do workspace (ClickUp)

- **Workspace (team):** `90171225448` — URL típica: `https://app.clickup.com/90171225448/...`
- **Space:** `90175627658` (`Space`)
- **Listas "Tarefas" por fase** (passar `workspace_id: "90171225448"` nas chamadas MCP se o default da sessão não for este workspace):

| Fase | Folder | List `Tarefas` |
|------|--------|----------------|
| 1 — Motor de Delay Básico | `90178732821` | `901713673870` |
| 2 — Feedback Engine | `90178732824` | `901713673872` |
| 3 — Modos Avançados | `90178732825` | `901713673875` |
| 4 — Spectral Ducking | `90178732826` | `901713673880` |
| 5 — Post-FX e Spatial | `90178732828` | `901713673882` |

## 1. Arranque de implementação (ticket → branch)

1. Resolver o **task id** (ex.: `86e1b2f6r` ou URL `https://app.clickup.com/t/86e1b2f6r`): `clickup_search` ou `clickup_get_task` com esse id.
2. **`git fetch`** e criar branch a partir da base acordada (normalmente `main`):

   - Nome sugerido: `cu/<taskId>-<slug-curto>` (ex.: `cu/86e1b2f6r-buffer-circular`), em minúsculas, sem espaços; se existir **custom_id** (ex. `DEV-42`), podes usar `cu/dev-42-slug`.

3. **ClickUp — estado “a trabalhar”:** chamar `clickup_update_task` com `task_id` e `status` **apenas** se existir um estado intermédio na lista (ex. *in progress*). Neste workspace, as tarefas amostradas usam sobretudo **`to do`** e **`complete`**; se não houver estado dedicado, regista o arranque com **`clickup_create_task_comment`** (ex.: “Branch: `cu/86e1b2f6r-…` (base `main`).”).

## 2. Abrir PR e ligar ao ClickUp

1. Push da branch: `git push -u origin HEAD`.
2. Criar PR com **`gh pr create`**; no **título ou corpo** incluir o link da tarefa ClickUp (`https://app.clickup.com/t/<id>`).
3. **Obrigatório:** deixar o URL do PR visível na tarefa:

   - Preferência: **`clickup_create_task_comment`** com texto tipo `PR: https://github.com/<org>/<repo>/pull/<n>` (e opcionalmente número `#n`).
   - Se a equipa quiser o link no topo da descrição: `clickup_update_task` com `markdown_description` **preservando** o conteúdo existente (ler antes com `clickup_get_task` + `detail_level: detailed`) e **prefixar** uma secção `## GitHub` com o link do PR.

4. Estado “em revisão”: se a lista tiver status adequado (ex. *in review*), `clickup_update_task` com `status: "<nome exacto>"`. Caso contrário, o comentário do PR pode bastar.

## 3. Após merge do PR no GitHub

1. Confirmar merge: `gh pr view <n> --json state,mergedAt` ou página do PR.
2. **Encerrar no ClickUp:** `clickup_update_task` com `status: "complete"` **quando** esse for o nome válido de concluído na lista (confirmado nas tarefas já fechadas do mesmo projeto; ver `reference.md`).
3. Comentário final opcional: `clickup_create_task_comment` — “Merged: `<url do PR>` (commit / data).”

## 4. Git / GitHub (repo)

- **Sem commits** a menos que o utilizador peça explicitamente (regra do utilizador).
- **PR:** seguir o fluxo do utilizador com `gh` (`gh pr create`, etc.).
- Corpo do PR: checklist mínima (build, testes) + link ClickUp; alinhar com [`Plans/00-cpp-clean-code.md`](../../../Plans/00-cpp-clean-code.md) se aplicável.

## Ferramentas MCP usuais

| Objetivo | Tool |
|----------|------|
| Árvore space/folder/list | `clickup_get_workspace_hierarchy` (`workspace_id: "90171225448"`, `max_depth: 2`) |
| Detalhe / descrição | `clickup_get_task` |
| Listar tarefas por lista(s) | `clickup_filter_tasks` (`list_ids`, `workspace_id`) |
| Actualizar estado ou descrição | `clickup_update_task` |
| Link do PR visível e notificação leve | `clickup_create_task_comment` |

**Nota:** `clickup_add_task_link` liga **tarefa a tarefa**, não URL GitHub; para PR usa comentário ou descrição.

## Quando o MCP ou `gh` falham

Indica o bloqueio (auth ClickUp, `gh auth`, remote errado) e deixa os comandos exactos para o utilizador concluir manualmente, sem assumir que o ticket foi actualizado.
