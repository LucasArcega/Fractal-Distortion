# Referência — ClickUp Fractal-Distortion (MCP)

Dados obtidos via `clickup_get_workspace_hierarchy` e `clickup_filter_tasks` / `clickup_get_task` com `workspace_id: "90171225448"` (actualizar este ficheiro se a estrutura ClickUp mudar).

## Workspace

| Campo | ID |
|--------|-----|
| Workspace (URL `app.clickup.com/<id>`) | `90171225448` |
| Space | `90175627658` |

## Pastas (folder) e listas `Tarefas`

| Fase | Folder ID | List "Tarefas" ID | URL da lista (vista) |
|------|-----------|-------------------|----------------------|
| Fase 1 — Motor de Delay Básico | `90178732821` | `901713673870` | `https://app.clickup.com/90171225448/v/l/li/901713673870` |
| Fase 2 — Feedback Engine | `90178732824` | `901713673872` | `https://app.clickup.com/90171225448/v/l/li/901713673872` |
| Fase 3 — Modos Avançados | `90178732825` | `901713673875` | `https://app.clickup.com/90171225448/v/l/li/901713673875` |
| Fase 4 — Spectral Ducking | `90178732826` | `901713673880` | `https://app.clickup.com/90171225448/v/l/li/901713673880` |
| Fase 5 — Post-FX e Spatial | `90178732828` | `901713673882` | `https://app.clickup.com/90171225448/v/l/li/901713673882` |

Lista extra no Space: `901713673379` (`List`).

## Estados de tarefa (amostra API)

Em tarefas inspeccionadas nas listas acima, o campo `status` (string) apareceu como:

- **`to do`** — trabalho por iniciar ou em curso sem outro estado.
- **`complete`** — tarefa concluída (ex.: ticket “Configurar projeto JUCE do zero”).

O objecto `status` em `clickup_get_task` (detalhe) inclui também `id`, `color`, `type` (ex. `open`). Para **mudar** estado, `clickup_update_task` aceita o **nome** do estado como string; o nome tem de existir na **lista** onde a tarefa vive — se a API devolver erro, confirmar nomes na UI ClickUp (Lista → … → statuses) ou adicionar estados (*in progress*, *in review*) à lista.

## IDs de tarefa (formato)

- Formato curto na URL: `https://app.clickup.com/t/86e1b2f6r` → `task_id` = `86e1b2f6r`.
- `clickup_update_task` / comentários aceitam também **custom id** se existir.

## Última sincronização desta referência

- Data: 2026-05-15  
- Fonte: MCP `user-clickup` (`clickup_get_workspace_hierarchy`, `clickup_filter_tasks`, `clickup_get_task`).
