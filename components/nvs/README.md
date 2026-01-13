# nvs_flash_5.1.1



## Getting started

To make it easy for you to get started with GitLab, here's a list of recommended next steps.

Already a pro? Just edit this README.md and make it your own. Want to make it easy? [Use the template at the bottom](#editing-this-readme)!

## Add your files

- [ ] [Create](https://docs.gitlab.com/ee/user/project/repository/web_editor.html#create-a-file) or [upload](https://docs.gitlab.com/ee/user/project/repository/web_editor.html#upload-a-file) files
- [ ] [Add files using the command line](https://docs.gitlab.com/ee/gitlab-basics/add-file.html#add-a-file-using-the-command-line) or push an existing Git repository with the following command:

```
cd existing_repo
git remote add origin https://gitlab.bekencorp.com/wifi/customer/nvs_flash_5.1.1.git
git branch -M main
git push -uf origin main
```

## Integrate with your tools

- [ ] [Set up project integrations](https://gitlab.bekencorp.com/wifi/customer/nvs_flash_5.1.1/-/settings/integrations)

## Collaborate with your team

- [ ] [Invite team members and collaborators](https://docs.gitlab.com/ee/user/project/members/)
- [ ] [Create a new merge request](https://docs.gitlab.com/ee/user/project/merge_requests/creating_merge_requests.html)
- [ ] [Automatically close issues from merge requests](https://docs.gitlab.com/ee/user/project/issues/managing_issues.html#closing-issues-automatically)
- [ ] [Enable merge request approvals](https://docs.gitlab.com/ee/user/project/merge_requests/approvals/)
- [ ] [Automatically merge when pipeline succeeds](https://docs.gitlab.com/ee/user/project/merge_requests/merge_when_pipeline_succeeds.html)

## Test and Deploy

Use the built-in continuous integration in GitLab.

- [ ] [Get started with GitLab CI/CD](https://docs.gitlab.com/ee/ci/quick_start/index.html)
- [ ] [Analyze your code for known vulnerabilities with Static Application Security Testing(SAST)](https://docs.gitlab.com/ee/user/application_security/sast/)
- [ ] [Deploy to Kubernetes, Amazon EC2, or Amazon ECS using Auto Deploy](https://docs.gitlab.com/ee/topics/autodevops/requirements.html)
- [ ] [Use pull-based deployments for improved Kubernetes management](https://docs.gitlab.com/ee/user/clusters/agent/)
- [ ] [Set up protected environments](https://docs.gitlab.com/ee/ci/environments/protected_environments.html)

***

# Editing this README

When you're ready to make this README your own, just edit this file and use the handy template below (or feel free to structure it however you want - this is just a starting point!).  Thank you to [makeareadme.com](https://www.makeareadme.com/) for this template.

## Suggestions for a good README
Every project is different, so consider which of these sections apply to yours. The sections used in the template are suggestions for most open source projects. Also keep in mind that while a README can be too long and detailed, too long is better than too short. If you think your README is too long, consider utilizing another form of documentation rather than cutting out information.

## Name
Choose a self-explaining name for your project.

## Description
Let people know what your project can do specifically. Provide context and add a link to any reference visitors might be unfamiliar with. A list of Features or a Background subsection can also be added here. If there are alternatives to your project, this is a good place to list differentiating factors.

## Badges
On some READMEs, you may see small images that convey metadata, such as whether or not all the tests are passing for the project. You can use Shields to add some to your README. Many services also have instructions for adding a badge.

## Visuals
Depending on what you are making, it can be a good idea to include screenshots or even a video (you'll frequently see GIFs rather than actual videos). Tools like ttygif can help, but check out Asciinema for a more sophisticated method.

## Installation
Within a particular ecosystem, there may be a common way of installing things, such as using Yarn, NuGet, or Homebrew. However, consider the possibility that whoever is reading your README is a novice and would like more guidance. Listing specific steps helps remove ambiguity and gets people to using your project as quickly as possible. If it only runs in a specific context like a particular programming language version or operating system or has dependencies that have to be installed manually, also add a Requirements subsection.

## Usage
Use examples liberally, and show the expected output if you can. It's helpful to have inline the smallest example of usage that you can demonstrate, while providing links to more sophisticated examples if they are too long to reasonably include in the README.

## Support
Tell people where they can go to for help. It can be any combination of an issue tracker, a chat room, an email address, etc.

## Roadmap
If you have ideas for releases in the future, it is a good idea to list them in the README.

## Contributing
State if you are open to contributions and what your requirements are for accepting them.

For people who want to make changes to your project, it's helpful to have some documentation on how to get started. Perhaps there is a script that they should run or some environment variables that they need to set. Make these steps explicit. These instructions could also be useful to your future self.

You can also document commands to lint the code or run tests. These steps help to ensure high code quality and reduce the likelihood that the changes inadvertently break something. Having instructions for running tests is especially helpful if it requires external setup, such as starting a Selenium server for testing in a browser.

## Authors and acknowledgment
Show your appreciation to those who have contributed to the project.

## License
For open source projects, say how it is licensed.

## Project status
If you have run out of energy or time for your project, put a note at the top of the README saying that development has slowed down or stopped completely. Someone may choose to fork your project or volunteer to step in as a maintainer or owner, allowing your project to keep going. You can also make an explicit request for maintainers.

## NVS encrypted partition workflow

### Step 0: Generate encrypted NVS binary
1. Run the partition generator with key creation:
   ```bash
   python3 ./nvs_partition_gen.py encrypt ./sample_singlepage_blob.csv nvs_enc.bin 0x10000 --keygen --keyfile key.bin
   ```
2. The generator script lives in `nvs_flash_5.1.1/nvs_partition_generator`.
3. The tool creates a V2 blob (with multipage support) and emits `nvs_enc.bin`.
4. Ensure you use the correct Python interpreter version for the generator.
5. Match the `0x10000` partition size to your actual NVS partition size; mismatched sizes can cause errors.

### Step 1: Encrypt the key file with the flash AES key
1. Obtain the flash AES key provisioned when `Security_Data_Enable` is active; for example:
   `73c7bf397f2ad6bf4e7403a7b965dc5ce0645df039c2d69c814ffb403183fb18`.
2. Determine the physical address of the `nvs_key` partition (e.g., `0x3e2000`).
3. Encrypt `keys.bin` with the flash key:
   ```bash
   ./beken_aes encrypt -infile keys.bin -startaddress 0x3a7860 \
     -keywords 73c7bf397f2ad6bf4e7403a7b965dc5ce0645df039c2d69c814ffb403183fb18 \
     -outfile keys_enc.bin
   ```
4. Note: `startaddress` is actually a virtual address. Calculate it as `vir_addr = (partition_addr + 33) / 34 * 32`.

### Step 2: Burn the binaries to flash
1. Write `nvs_enc.bin` to the NVS partition start (physical address).
2. For the key, burn `key_enc_crc.bin` instead of the raw `key.bin`, and align it to the required physical address (which may differ from the `nvs_key` partition start).
	

## Thread safety stress test

The new `components/nvs/src/nvs_thread_safety_test.c` file spins up three concurrent tasks that exercise `nvs_set_i32`, `nvs_get_i32`, and `nvs_erase_all` against the same namespace/key pair (`dummyNamespace`/`threadSafeKey`). The workload mirrors your command-line sequence (`nvs -s ...`, `nvs -g ...`, `nvs -e ...`) while keeping the operations inside the RTOS scheduler.

1. Call `nvs_thread_safety_test_start()` from a CLI command or regression harness to initialize NVS, open the namespace, and launch the writer/reader/eraser trio.
2. The writer cycles through signed integer values (`12345/-12345/66666666/-66666666`, etc.) and commits the last value to `threadSafeKey`.
3. The reader samples the same key at high frequency and logs mismatches when they indicate an in-flight update rather than an error.
4. The eraser periodically runs `nvs_erase_all()` to force a namespace reset, ensuring the other tasks recover and continue to succeed.
5. Once you finish the stress run, call `nvs_thread_safety_test_stop()` to signal each task to exit and to deinitialize the flash layer.

Use `nvs_thread_safety_test_is_running()` to gate repeated start/stop cycles.

### CLI configuration

The CLI exposes the `nvs_race_test` command when `CONFIG_SUPPORT_NVS_THREAD_SAFETY_TEST` is enabled.

**Required Configuration Options:**

To enable the CLI test functionality, the following configuration options must be enabled in your `config` file:

```kconfig
CONFIG_SUPPORT_NVS_FLASH=y
CONFIG_NVS_TEST=y
CONFIG_SUPPORT_NVS_THREAD_SAFETY_TEST=y
CONFIG_SUPPORT_PARTITIONS_PARTITION=y
CONFIG_FLASH_PARTITION_USER=y
CONFIG_NVS_ENCRYPTION=y
CONFIG_CLI=y
```

**Command Usage:**

```sh
nvs_race_test --start <read_task_cnt> <write_task_cnt>
nvs_race_test --stop
nvs_race_test --stats
nvs_race_test --help
```

**Command Options:**
- `--start <read_task_cnt> <write_task_cnt>`: Start the thread safety test with specified number of read and write tasks
- `--stop`: Stop the running thread safety test and display final statistics
- `--stats`: Display current operation statistics (read/write success/failure counts)
- `--help`: Display help message with usage examples

**Examples:**
```sh
nvs_race_test --start 2 3    # Create 2 read tasks and 3 write tasks
nvs_race_test --stats        # View current statistics
nvs_race_test --stop         # Stop the test (shows final statistics)
```

**Task Behavior:**
- Each read task performs all 9 read operations covering: u8, i8, u16, i16, u32, i32, str, blob (dummyHex2BinKey), blob (dummyBase64Key)
- Each write task performs all 9 write operations with corresponding values
- Tasks are created in priority order (lower priority tasks first, then higher priority tasks)
- Each task has a unique priority: `NVS_RACE_BASE_PRIORITY + task_index`
- Task names are unique and indexed: `nvs_reader_<idx>` and `nvs_writer_<idx>` for easier debugging

Behind the scenes, the command calls `nvs_thread_safety_test_start_custom()` with the specified task counts. The `nvs_thread_safety_test_start_custom()` API is also exposed for programmatic control, allowing automated scripts to inject arbitrary configurations.


