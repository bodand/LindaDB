load('cirrus', 'env', 'http')


def on_build_completed(_ctx):
    completed_release(_ctx)


def on_build_failed(_ctx):
    failed_release(_ctx)


def failed_release(_ctx):
    if env.get("CIRRUS_TAG") is not None:
        delete_current_release()


def completed_release(_ctx):
    if env.get("CIRRUS_TAG") is not None:
        finish_current_release()


def repo_path():
    repo = env.get("CIRRUS_REPO_NAME")
    assert (repo is not None)
    return "/repos/bodand/cg3"


def delete_current_release():
    """Deletes the current release."""
    rel_id = get_current_release()
    print("deleting release with id {}".format(rel_id))
    delete_release(rel_id)


def finish_current_release():
    """Updates the current release from draft to complete release."""
    rel_id = get_current_release()
    print("finishing release with id {}".format(rel_id))
    return finish_release(rel_id)


def get_current_release():
    releases = github_get(repo_path() + "/releases").json()
    for rel in releases:
        if rel["draft"]:
            print("found draft for tag: '{}', id: '{}'".format(rel["tag_name"], rel["id"]))
            return "{}".format(rel["id"])
    return None


def delete_release(id):
    github_delete(repo_path() + "/releases/" + id)


def finish_release(id):
    github_patch(repo_path() + "/releases/" + id, {"draft": False})


def get_github_token():
    return env.get("GITHUB_TOKEN")


def github_delete(url):
    hdrs = {
        "Accept": "application/vnd.github+json",
        "Authorization": "Bearer " + get_github_token(),
        "X-GitHub-Api-Version": "2022-11-28"
    }
    http.delete("https://api.github.com" + url, headers=hdrs)


def github_get(url):
    hdrs = {
        "Accept": "application/vnd.github+json",
        "Authorization": "Bearer " + get_github_token(),
        "X-GitHub-Api-Version": "2022-11-28"
    }
    return http.get("https://api.github.com" + url, headers=hdrs)


def github_patch(url, body):
    hdrs = {
        "Accept": "application/vnd.github+json",
        "Authorization": "Bearer " + get_github_token(),
        "X-GitHub-Api-Version": "2022-11-28"
    }
    return http.patch("https://api.github.com" + url, headers=hdrs, json_body=body)
