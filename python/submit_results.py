#!/usr/bin/env python3

import sys
import argparse
import datetime

import boto3 

client = boto3.client('dynamodb')
res = boto3.resource('dynamodb')

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
        n = 0
        for line in f:
            n += 1
            try:
                name, instructions, branches = line.strip().split('\t')
            except ValueError as e:
                sys.stderr.write("Malformed results file (line {})\n".format(n))
                sys.exit(1)
            d[name] = {
                        'instructions': int(instructions),
                        'branches': int(branches)
                      }
    return d

if __name__ == '__main__':
    parser = argparse.ArgumentParser(description='Submit results to DynamoDB')
    parser.add_argument('results', type=str)
    parser.add_argument('-c', '--commit', type=str, required=True,
            help='Commit that these results are associated with.',
            dest='commit')
    args = parser.parse_args()

    o = parse_results(args.results)

    client = boto3.client('dynamodb')
    resource = boto3.resource('dynamodb')

    o['commit'] = args.commit
    o['timestamp'] = datetime.datetime.now().isoformat()

    table = get_table('screen')
    res = table.put_item(Item=o)
    print(res)


