#!/usr/bin/env python3

import sys
import argparse
import datetime
import json

try:
    import requests
except ImportError:
    import pip
    pip.main(['install', 'requests'])

    import requests

BASE_URL = 'https://screen-web.herokuapp.com'

def publish_results(owner_repo, commit, report):
    res = requests.put(
        BASE_URL + '/{owner_repo}/publish/{commit}'.format(
            owner_repo=owner_repo,
            commit=commit,
        ),
        json=report,
    )
    # response is always json, unless network is dead
    return res.json()

def parse_results(filename):
    '''
    Consume a screen results file and return a dictionary describing it.
    '''
    d = {}
    with open(filename, 'r') as f:
        contents = f.read()
        print("Loading: ", contents)
        f.seek(0)
        results = json.load(f)
        for obj in results:
            d.update(obj)
    return d

if __name__ == '__main__':
    parser = argparse.ArgumentParser(description='Submit results to DynamoDB')
    parser.add_argument('results', type=str)
    parser.add_argument('-c', '--commit', type=str, required=True,
            help='Commit that these results are associated with.',
            dest='commit')
    parser.add_argument('-o', '-r', '--owner-repo', type=str, required=True,
                        help='Repository identifier (in format OWNER/REPO) '
                        'for which the analysis was performed')
    args = parser.parse_args()

    results = parse_results(args.results)

    res = publish_results(
        owner_repo=args.owner_repo,
        commit=args.commit,
        report=results,
    )
    print(res)


