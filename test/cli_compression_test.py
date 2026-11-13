#!/usr/bin/env python3
"""
@file
@author Dominik Loidolt (dominik.loidolt@univie.ac.at)
@date   2025
@copyright GPL-2.0

@brief AIRSPACE CLI Compression Tests
"""

import os
import unittest
from pathlib import Path

import clitest
from clitest import RETURN_FAILURE, RETURN_SUCCESS, CliTest

DATA_FILE1 = bytes.fromhex("0001 0002")
DATA_FILE2 = bytes.fromhex("0003 0004")


class TestCompression(unittest.TestCase):
    @classmethod
    def setUpClass(cls):
        cls.cli_test = CliTest()
        cls.airspace = cls.cli_test.run_cli
        cls.CMP_HDR_SIZE = cls.get_hdr_size(cls)

    def setUp(self):
        test_name = self.id().split(".")[-1]
        self.test_dir = self.cli_test.change_test_directory(test_name)
        self.file1 = self.test_dir / "file_1.bin"
        self.file2 = self.test_dir / "file_2.bin"
        self.file1.write_bytes(DATA_FILE1)
        self.file2.write_bytes(DATA_FILE2)

    def get_hdr_size(self):
        result = self.airspace(["-c", "--stdout"], stdin=DATA_FILE1)
        if result.returncode != RETURN_SUCCESS:
            raise RuntimeError("Can not calculate header size.")

        return len(result.stdout) - len(DATA_FILE1)

    def get_compressed_file_data(self, original_file: Path):
        cmp_file = original_file.with_name(original_file.name + ".air")
        self.assertTrue(cmp_file.exists())
        return cmp_file.read_bytes()

    def test_compress_2_files_to_dev_null(self):
        result = self.airspace(
            ["-c", self.file1, self.file2, "-o", os.devnull, "--quiet"]
        )

        self.assertCli(result)

    def test_compress_2_files_to_stdout(self):
        result = self.airspace(["-c", self.file1, self.file2, "--stdout"])

        self.assertCli(
            result,
            stdout_exp=DATA_FILE1,
            stdout_match_mode="contains",
        )
        offset = 2 * self.CMP_HDR_SIZE + len(DATA_FILE1)
        self.assertEqual(DATA_FILE2, result.stdout[offset:])

    def test_compress_2_files_normally(self):
        result = self.airspace(["-c", self.file1, self.file2, "--quiet"])

        self.assertCli(result)
        cmp_file1 = self.get_compressed_file_data(self.file1)
        cmp_file2 = self.get_compressed_file_data(self.file2)
        self.assertEqual(DATA_FILE1, cmp_file1[self.CMP_HDR_SIZE :], result.args)
        self.assertEqual(DATA_FILE2, cmp_file2[self.CMP_HDR_SIZE :], result.args)

    def test_compress_data_from_stdin_to_stdout(self):
        for arg in [["-"], []]:
            with self.subTest(arg=arg):
                result = self.airspace(["-c"] + arg, stdin=DATA_FILE1)

                self.assertCli(
                    result,
                    stdout_exp=DATA_FILE1,
                    stdout_match_mode="contains",
                )

    def test_compress_file_to_a_output_file(self):
        cmp_file = self.test_dir / "output.air"

        result = self.airspace(["-c", self.file1, "-o", cmp_file, "--quiet"])

        self.assertCli(result)
        cmp_data = cmp_file.read_bytes()[self.CMP_HDR_SIZE :]
        self.assertEqual(DATA_FILE1, cmp_data, result.args)

    def test_compress_file_and_data_from_stdin(self):
        result = self.airspace(["-c", self.file1, "-", "--quiet"], stdin=DATA_FILE2)

        self.assertCli(
            result,
            stdout_exp=DATA_FILE1,
            stdout_match_mode="contains",
        )
        offset = 2 * self.CMP_HDR_SIZE + len(DATA_FILE1)
        self.assertEqual(DATA_FILE2, result.stdout[offset:])

    def test_compress_files_with_different_sizes(self):
        small_file = self.test_dir / "small_file.bin"
        small_file.write_bytes(bytes.fromhex("0003"))

        result = self.airspace(["-c", self.file1, small_file, "--quiet"])

        self.assertCli(result)
        cmp_file1 = self.get_compressed_file_data(self.file1)
        cmp_small_file = self.get_compressed_file_data(small_file)
        self.assertEqual(DATA_FILE1, cmp_file1[self.CMP_HDR_SIZE :], result.args)
        self.assertEqual(
            bytes.fromhex("0003"), cmp_small_file[self.CMP_HDR_SIZE :], result.args
        )

    def test_abort_when_reading_from_stdin_on_console(self):
        for arg in [["-"], []]:
            with self.subTest(arg=arg):
                result = self.airspace(
                    arg + ["-c", "--debug-stdin-is-consol"], stdin=DATA_FILE1
                )

                self.assertCli(
                    result,
                    returncode_exp=RETURN_FAILURE,
                    stderr_exp="stdin",
                    stderr_match_mode="contains",
                )

    def test_abort_when_writing_to_stdout_on_console(self):
        for arg in [["-"], []]:
            with self.subTest(arg=arg):
                result = self.airspace(
                    arg + ["-c", "--debug-stdout-is-consol"], stdin=DATA_FILE1
                )

                self.assertCli(
                    result,
                    returncode_exp=RETURN_FAILURE,
                    stderr_exp="stdout",
                    stderr_match_mode="contains",
                )

    def test_compress_with_identifier(self):
        result = self.airspace(
            ["-c", "--identifier", "1413829460", self.file1, "--quiet"]
        )

        self.assertCli(result)

        cmp_file = self.get_compressed_file_data(self.file1)
        self.assertEqual(DATA_FILE1, cmp_file[self.CMP_HDR_SIZE :])
        expected_identifier_bytes = (1413829460).to_bytes(4, byteorder="big")
        self.assertIn(expected_identifier_bytes, cmp_file[: self.CMP_HDR_SIZE])

    def test_not_overwrite_existing_file(self):
        existing_file = self.test_dir / "existing_file.txt"
        existing_file.write_text("Do not overwrite this file!")

        result = self.airspace(["-c", self.file1, self.file2, "-o", existing_file])

        self.assertCli(
            result,
            returncode_exp=RETURN_FAILURE,
            stderr_exp="already exists",
            stderr_match_mode="contains",
        )

    def test_not_overwrite_existing_directory(self):
        existing_dir = self.test_dir / "existing_dir"
        existing_dir.mkdir()

        result = self.airspace(["-c", self.file1, self.file2, "-o", existing_dir])

        self.assertCli(
            result,
            returncode_exp=RETURN_FAILURE,
            stderr_exp="is a directory",
            stderr_match_mode="contains",
        )

    def test_not_overwrite_input_file(self):
        result = self.airspace(["-c", self.file1, "-o", self.file1])

        self.assertCli(
            result,
            returncode_exp=RETURN_FAILURE,
            stderr_exp="already exists",
            stderr_match_mode="contains",
        )


if __name__ == "__main__":
    clitest.main()
