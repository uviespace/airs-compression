#!/usr/bin/env python3
"""
@file
@author Dominik Loidolt (dominik.loidolt@univie.ac.at)
@date   2025
@copyright GPL-2.0

@brief AIRSPACE CLI Basic Option Tests
"""

import unittest
from pathlib import Path

import clitest
from clitest import RETURN_FAILURE, CliTest
from get_library_version import extract_version

CMP_H_PATH = Path(__file__).resolve().parent.joinpath("../lib/cmp.h")
VERSION_STR_EXP = extract_version(CMP_H_PATH)


class TestBasicOptions(unittest.TestCase):
    @classmethod
    def setUpClass(cls):
        cls.cli_test = CliTest()
        cls.airspace = cls.cli_test.run_cli

    def test_print_version_string_to_stdout(self):
        for arg in [["-V"], ["--version"], ["-qvV"]]:
            with self.subTest(arg=arg):
                result = self.airspace(arg)

                self.assertCli(
                    result, stdout_exp=VERSION_STR_EXP, stdout_match_mode="contains"
                )

    def test_print_version_minimal_string_to_stdout(self):
        for arg in [["-qV"], ["--quiet", "--version"], ["-q", "-V"]]:
            with self.subTest(arg=arg):
                result = self.airspace(arg)

                self.assertCli(result, stdout_exp=VERSION_STR_EXP + "\n")

    def test_print_help_text(self):
        for arg in [["-h"], ["--help"]]:
            with self.subTest(arg=arg):
                result = self.airspace(arg)

                self.assertCli(
                    result, stdout_exp="Usage: ", stdout_match_mode="contains"
                )

    def test_invalid_long_options_are_detected(self):
        result = self.airspace("--my_invalid_option")

        self.assertCli(
            result,
            returncode_exp=RETURN_FAILURE,
            stderr_exp="my_invalid_option",
            stderr_match_mode="contains",
        )

    def test_invalid_short_options_are_detected(self):
        result = self.airspace("-u")

        self.assertCli(
            result,
            returncode_exp=RETURN_FAILURE,
            stderr_exp="u",
            stderr_match_mode="contains",
        )

    def test_o_option_needs_an_argument(self):
        result = self.airspace(["-c", "-o"], stdin=bytes.fromhex("0001 0002"))

        self.assertCli(
            result,
            returncode_exp=RETURN_FAILURE,
            stderr_exp="requires an argument",
            stderr_match_mode="contains",
        )


if __name__ == "__main__":
    clitest.main()
