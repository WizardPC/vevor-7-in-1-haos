"""Composant externe ESPHome : Vevor 7-in-1 (868 MHz) via CC1101.

Toute la logique de décodage vit en C++ (vevor_frame.* / vevor_pcm.*) ; les
fichiers Python ne font que déclarer les entités et câbler le composant.
"""

import esphome.codegen as cg
from esphome.components import remote_base

CODEOWNERS = ["@WizardPC"]
# Le lien avec le CC1101 est matériel (GDO0), mais c'est le remote_receiver qui
# fournit les timings bruts : le composant ne peut pas fonctionner sans lui.
DEPENDENCIES = ["remote_receiver"]
AUTO_LOAD = ["sensor", "binary_sensor", "text_sensor"]

vevor_7in1_ns = cg.esphome_ns.namespace("vevor_7in1")

Vevor7in1Component = vevor_7in1_ns.class_(
    "Vevor7in1Component", cg.PollingComponent, remote_base.RemoteReceiverListener
)
