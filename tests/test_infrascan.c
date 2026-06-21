/*
 * test_infrascan.c — REMOVED: infrascan tests already exist in test_pipeline.c.
 *
 * This file is intentionally empty. All infrascan tests are in test_pipeline.c
 * (infra_parse_dockerfile_*, infra_parse_dotenv*, infra_parse_shell*,
 *  infra_parse_terraform*, infra_is_*, infra_clean_json_brackets,
 *  infra_secret_detection, infra_qn_helper).
 */
#include "test_framework.h"

SUITE(infrascan) {
    /* All infrascan tests live in test_pipeline.c's pipeline suite */
}
