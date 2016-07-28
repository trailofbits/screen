#!/usr/bin/env python3

import sys
import argparse
import datetime
import json

try:
    import boto3 
except ImportError:
    import pip
    pip.main(['install', 'boto3'])

    import boto3

region = 'us-east-1'
client = boto3.client('dynamodb', region_name=region)
res = boto3.resource('dynamodb', region_name=region)

def get_table(table_name, key_name='commit'):
    try:
        desc = client.describe_table(TableName=table_name)
    except: # Table does not exist
        table = res.create_table(
          TableName = table_name,
          KeySchema            =[{'AttributeName': 'timestamp','KeyType': 'RANGE'},
                                 {'AttributeName': 'commit'   ,'KeyType': 'HASH' } 
                                ],
          AttributeDefinitions =[{'AttributeName': 'timestamp','AttributeType': 'S' },
                                 {'AttributeName': 'commit'   ,'AttributeType': 'S' },
                                ],
          ProvisionedThroughput={'ReadCapacityUnits': 5,'WriteCapacityUnits': 5}
        )
        table.meta.client.get_waiter('table_exists').wait(TableName=table_name)

    table = res.Table(table_name)
    print(table.scan())
    return table


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
    args = parser.parse_args()

    results = parse_results(args.results)

    entry = {}
    entry['commit'] = args.commit
    entry['timestamp'] = datetime.datetime.now().isoformat()
    entry['results'] = results

    table = get_table('screen')
    res = table.put_item(Item=entry)
    print(res)


