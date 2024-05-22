// Copyright (C) 2024  Paul Johnson

// This program is free software: you can redistribute it and/or modify
// it under the terms of the GNU Affero General Public License as
// published by the Free Software Foundation, either version 3 of the
// License, or (at your option) any later version.

// This program is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
// GNU Affero General Public License for more details.

// You should have received a copy of the GNU Affero General Public License
// along with this program.  If not, see <https://www.gnu.org/licenses/>.

#pragma once

struct rr_renderer;

void rr_triceratops_head_draw(struct rr_renderer *);
void rr_triceratops_body_draw(struct rr_renderer *);
void rr_triceratops_leg1_draw(struct rr_renderer *);
void rr_triceratops_leg2_draw(struct rr_renderer *);
void rr_triceratops_tail_draw(struct rr_renderer *);

void rr_pteranodon_wing1_draw(struct rr_renderer *);
void rr_pteranodon_wing2_draw(struct rr_renderer *);
void rr_pteranodon_body_draw(struct rr_renderer *);

void rr_t_rex_body_draw(struct rr_renderer *);
void rr_t_rex_leg1_draw(struct rr_renderer *);
void rr_t_rex_leg2_draw(struct rr_renderer *);
void rr_t_rex_head_draw(struct rr_renderer *);
void rr_t_rex_tail_draw(struct rr_renderer *);

void rr_dakotaraptor_body_draw(struct rr_renderer *);
void rr_dakotaraptor_wing1_draw(struct rr_renderer *);
void rr_dakotaraptor_wing2_draw(struct rr_renderer *);
void rr_dakotaraptor_head_draw(struct rr_renderer *);
void rr_dakotaraptor_tail_draw(struct rr_renderer *);

void rr_fern_draw(struct rr_renderer *);
void rr_tree_draw(struct rr_renderer *);
void rr_meteor_draw(struct rr_renderer *);
void rr_beehive_draw(struct rr_renderer *);

void rr_pachycephalosaurus_head_draw(struct rr_renderer *);
void rr_pachycephalosaurus_body_draw(struct rr_renderer *);
void rr_pachycephalosaurus_leg1_draw(struct rr_renderer *);
void rr_pachycephalosaurus_leg2_draw(struct rr_renderer *);
void rr_pachycephalosaurus_tail_draw(struct rr_renderer *);

void rr_ornithomimus_body_draw(struct rr_renderer *);
void rr_ornithomimus_wing1_draw(struct rr_renderer *);
void rr_ornithomimus_wing2_draw(struct rr_renderer *);
void rr_ornithomimus_head_draw(struct rr_renderer *);
void rr_ornithomimus_tail_draw(struct rr_renderer *);

void rr_ankylosaurus_head_draw(struct rr_renderer *);
void rr_ankylosaurus_body_draw(struct rr_renderer *);
void rr_ankylosaurus_tail_draw(struct rr_renderer *);

void rr_quetzalcoatlus_body_draw(struct rr_renderer *);
void rr_quetzalcoatlus_head_draw(struct rr_renderer *);
void rr_quetzalcoatlus_wing1_draw(struct rr_renderer *);
void rr_quetzalcoatlus_wing2_draw(struct rr_renderer *);

void rr_edmontosaurus_body_draw(struct rr_renderer *);
void rr_edmontosaurus_leg1_draw(struct rr_renderer *);
void rr_edmontosaurus_leg2_draw(struct rr_renderer *);
void rr_edmontosaurus_head_draw(struct rr_renderer *);
void rr_edmontosaurus_tail_draw(struct rr_renderer *);

void rr_ant_abdomen_draw(struct rr_renderer *);
void rr_ant_head_draw(struct rr_renderer *);
void rr_ant_thorax_draw(struct rr_renderer *);
void rr_ant_leg_draw(struct rr_renderer *);

void rr_hornet_abdomen_draw(struct rr_renderer *);
void rr_hornet_head_draw(struct rr_renderer *);
void rr_hornet_thorax_draw(struct rr_renderer *);
void rr_hornet_leg_draw(struct rr_renderer *);
void rr_hornet_wing_draw(struct rr_renderer *);

void rr_dragonfly_abdomen_draw(struct rr_renderer *);
void rr_dragonfly_head_draw(struct rr_renderer *);
void rr_dragonfly_thorax_draw(struct rr_renderer *);
void rr_dragonfly_wing_draw(struct rr_renderer *);

void rr_honeybee_abdomen_draw(struct rr_renderer *);
void rr_honeybee_thorax_draw(struct rr_renderer *);
void rr_honeybee_head_draw(struct rr_renderer *);
void rr_honeybee_leg_draw(struct rr_renderer *);
void rr_honeybee_wing_draw(struct rr_renderer *);

void rr_spider_abdomen_draw(struct rr_renderer *);
void rr_spider_head_draw(struct rr_renderer *);
void rr_spider_leg_draw(struct rr_renderer *);

void rr_house_centipede_body_draw(struct rr_renderer *);
void rr_house_centipede_head_draw(struct rr_renderer *);
void rr_house_centipede_leg_draw(struct rr_renderer *);

void rr_hc_tile_1_draw(struct rr_renderer *);
void rr_hc_tile_2_draw(struct rr_renderer *);
void rr_hc_tile_3_draw(struct rr_renderer *);

void rr_ga_tile_1_draw(struct rr_renderer *);
void rr_ga_tile_2_draw(struct rr_renderer *);
void rr_ga_tile_3_draw(struct rr_renderer *);

void rr_oc_tile_1_draw(struct rr_renderer *);
void rr_oc_tile_2_draw(struct rr_renderer *);
void rr_oc_tile_3_draw(struct rr_renderer *);

void rr_prop_fern_draw(struct rr_renderer *);
void rr_prop_moss_draw(struct rr_renderer *);
void rr_prop_water_lettuce_draw(struct rr_renderer *);

void rr_stone_draw(struct rr_renderer *);