#!/usr/bin/env python3

import argparse
import json
import sys

try:
    import requests
except ImportError:
    import pip
    pip.main(['install', 'requests'])

    import requests

BASE_URL = 'https://screen-web.herokuapp.com'

def merge_dicts_from_file(filename):
    '''
    Consume a screen results file and return a dictionary describing it.
    '''
    d = {}
    with open(filename, 'r') as f:
        contents = f.read()
        print("Parsing JSON: ", contents)
        results = json.loads(contents)
        # It is an array of dicts,
        # each dict describing some subset of functions.
        # We want to merge all dicts into one.
        # FIXME: sometimes there are several reports for the same function?..
        for obj in results:
            d.update(obj)
    return d

def project(val):
    """ Validate value to make sure it has a slash """
    owner, _, repo = val.partition('/')
    if not owner or not repo:
        raise argparse.ArgumentTypeError(
            'please supply value in format OWNER/REPO',
        )
    return val

if __name__ == '__main__':
    parser = argparse.ArgumentParser(description='Submit results to DynamoDB')
    parser.add_argument('results', type=str,
                        help='Name of JSON file generated by screen')
    parser.add_argument('range_results', type=str,
                        help='Name of JSON file '
                        'generated by screen\'s range analysis')
    parser.add_argument('-c', '--commit', type=str, required=True,
                        help='Commit that these results are associated with.',
                        dest='commit')
    parser.add_argument('-p', '--project', type=project, required=True,
                        help='Repository identifier (in format OWNER/REPO) '
                        'for which the analysis was performed')
    parser.add_argument('-k', '--api-key', type=str, required=True,
                        help='API key for the project')
    args = parser.parse_args()

    report = merge_dicts_from_file(args.results)
    range_report = merge_dicts_from_file(args.range_results)

    # range report is a dict consisting of items like this:
    # "func-line": "invariant_string"
    # We want to put all invariants for certain func
    # into report for that func.
    for key, val in range_report.items():
        func, _, line = key.partition('-')
        # it's possible (maybe) that there is no "main" report for the func;
        # so let's make sure it exists, at least as empty dict
        report.setdefault(func, {})
        # also let's make sure that `invariants` field exists in the report
        report[func].setdefault('invariants', {})
        # now save this line
        report[func]['invariants'][line] = val

    print("Complete JSON:", json.dumps(report))
    res = requests.put(
        BASE_URL + '/{project}/publish/{commit}'.format(
            project=args.project,
            commit=args.commit,
        ),
        params={'key': args.api_key},
        json=report,
    )
    # response should always be json, unless network is dead
    print("Result:", res.json())
    if not res.ok:  # .ok is False for 4xx and 5xx codes
        # notify failure status
        sys.exit(1)
